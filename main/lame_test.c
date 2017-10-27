#include "lame.h"
#include "lame_test.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include <assert.h>
#include <esp_types.h>
#include <stdio.h>
#include "rom/ets_sys.h"
#include "esp_heap_alloc_caps.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include <sys/time.h>

#include "mount_sd.h"

#include "freertos/queue.h"

#include "esp_intr_alloc.h"
#include "esp_attr.h"
#include "driver/timer.h"
#include "driver/gpio.h"

#include "mcp3201.h"

extern const uint8_t Sample16kHz_raw_start[] asm("_binary_Sample16kHz_mono_8kHz_raw_start");
extern const uint8_t Sample16kHz_raw_end[]   asm("_binary_Sample16kHz_mono_8kHz_raw_end");

StaticQueue_t sample_queue;
QueueHandle_t sample_queue_handle;
bool data_sampler_stopped = false;

uint64_t timer_ticks = 0;
uint32_t lost_samples = 0;
short int *current_buffer_address = (short int *)Sample16kHz_raw_start;

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int32_t threshold = 400;
int32_t center = 1852;

static void timer_isr(void* arg)
{
    /* We have not woken a task at the start of the ISR. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;

    // your code, runs in the interrupt
    if (current_buffer_address < (short int *)Sample16kHz_raw_end)
    {
        int32_t val = mcp3201_get_value();
        if (val >= 2 * center)
        {
            val = (2 * center) - 1;
        }
        if (val < center - threshold || val > center + threshold)
        {
            val = center;
        }
        int16_t sample = map(val, 0, 2 * center, INT16_MIN, INT16_MAX);

        if (current_buffer_address + 1 >= (short int *)Sample16kHz_raw_end)
        {
            data_sampler_stopped = true;
        }
        if (!xQueueSendFromISR(sample_queue_handle, &sample, &xHigherPriorityTaskWoken))
        {
            lost_samples++;
        }
        current_buffer_address++;
    }

    if (xHigherPriorityTaskWoken)
    {
        /* Actual macro used here is port specific. */

        portYIELD_FROM_ISR();
    }

    timer_ticks++;
}

unsigned int get_data(short int *buffer, unsigned buffer_entry_count)
{
    for (unsigned int i = 0; i < buffer_entry_count; )
    {
        short int sample;
        if (xQueueReceive(sample_queue_handle, &sample, data_sampler_stopped ? 0 : portMAX_DELAY))
        {
            buffer[i++] = sample;
        }
        else
        {
            if (data_sampler_stopped && uxQueueMessagesWaiting(sample_queue_handle) == 0)
            {
                return i;
            }
            printf("Failed to receive item from queue\n");
        }
    }
    
    return buffer_entry_count;
}


struct esp_encoder
{
    lame_t lame;
    unsigned int sampleRate;
    int num_channels;
    short int *pcm_samples;
    int framesize;
    int total;
    int frames;
    int nsamples;
    unsigned char *mp3buf;
    int mp3buf_size;
};


esp_err_t esp_encoder__initialize(struct esp_encoder *encoder, unsigned int sampleRate, int num_channels)
{
    encoder->sampleRate = sampleRate;
    encoder->num_channels = num_channels;
    encoder->framesize = 0;
    encoder->total=0;
    encoder->frames=0;
    encoder->mp3buf_size=2000;  //mp3buf_size in bytes = 1.25*num_samples + 7200
    encoder->nsamples=1152;

    size_t free8start, free32start;

    free8start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_8BIT);
    free32start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_32BIT);
    printf("pre lame_init() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

    encoder->mp3buf=malloc(encoder->mp3buf_size);
    if (!encoder->mp3buf)
    {
        printf("Failed to allocated MP3 buffer.\n");
        return ESP_ERR_NO_MEM;
    }
    encoder->pcm_samples = malloc(encoder->nsamples * sizeof(short int));
    if (!encoder->pcm_samples)
    {
        printf("Failed to allocated sample buffer.\n");
        return ESP_ERR_NO_MEM;
    }

    /* Init lame flags.*/
    encoder->lame = lame_init();

    free8start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_8BIT);
    free32start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_32BIT);
    printf("post lame_init() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

    if (!encoder->lame)
    {
        printf("Unable to initialize lame object.\n");
        return ESP_FAIL;
    }
    else
    {
        printf("Able to initialize lame object.\n");
    }

    /* set other parameters.*/
    lame_set_VBR(encoder->lame, vbr_default);
    lame_set_num_channels(encoder->lame, encoder->num_channels);
    lame_set_in_samplerate(encoder->lame, encoder->sampleRate);
    lame_set_quality(encoder->lame, 7);  /* set for high speed and good quality. */
    lame_set_mode(encoder->lame, encoder->num_channels == 1 ? MONO : JOINT_STEREO);  /* audio is joint stereo */

    // lame_set_out_samplerate(lame, sampleRate);
    printf("Able to set a number of parameters too.\n");


    /* set more internal variables. check for failure.*/
    int initRet = lame_init_params(encoder->lame);
    if(initRet < 0)
    {
        printf("Fail in setting internal parameters with code=%d\n",initRet);
        return ESP_FAIL;
    }
    else
    {
        printf("OK setting internal parameters\n");
    }

    free8start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_8BIT);
    free32start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_32BIT);
    printf("post lame_init_params() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

    // lame_print_config(lame);
    // lame_print_internals(lame);

    encoder->framesize = lame_get_framesize(encoder->lame);
    printf("Framesize = %d\n", encoder->framesize);
    // assert(framesize <= 1152);

    return ESP_OK;
}


esp_err_t esp_encoder__encode_audio(struct esp_encoder *encoder, FILE *f)
{
    struct timeval tvalBefore, tvalFirstFrame, tvalAfter;
    int bytes_written;
    unsigned int samples_retrieved;
    int num_samples_encoded;


    gettimeofday (&tvalBefore, NULL);
    while ((samples_retrieved = get_data(encoder->pcm_samples, encoder->nsamples)) > 0)
    {
        //  printf("\n=============== lame_encode_buffer_interleaved================ \n");
        /* encode samples. */
        if (samples_retrieved < encoder->nsamples) {
            memset(encoder->pcm_samples + samples_retrieved, 0, sizeof(short int) * (encoder->nsamples - samples_retrieved));
        }

        if (encoder->num_channels == 1)
        {
            // Mono
            num_samples_encoded = lame_encode_buffer(encoder->lame, encoder->pcm_samples, encoder->pcm_samples, encoder->nsamples, encoder->mp3buf, encoder->mp3buf_size);
        }
        else
        {
            // Stereo
            // Pass in the number of samples in one channel. That is the total number of samples divided by num_channels.
            num_samples_encoded = lame_encode_buffer_interleaved(encoder->lame, encoder->pcm_samples, encoder->nsamples / encoder->num_channels, encoder->mp3buf, encoder->mp3buf_size);
        }

        //   printf("number of samples encoded = %d pcm_samples %p \n", num_samples_encoded, pcm_samples);

        if (encoder->total==0)
        {
            gettimeofday (&tvalFirstFrame, NULL);
        }

        /* check for value returned.*/
        if(num_samples_encoded > 1)
        {
            //printf("It seems the conversion was successful.\n");
            encoder->total+=num_samples_encoded;
        }
        else if(num_samples_encoded == -1)
        {
            printf("mp3buf was too small\n");
            return ESP_FAIL;
        }
        else if(num_samples_encoded == -2)
        {
            printf("There was a malloc problem.\n");
            return ESP_FAIL;
        }
        else if(num_samples_encoded == -3)
        {
            printf("lame_init_params() not called.\n");
            return ESP_FAIL;
        }
        else if(num_samples_encoded == -4)
        {
            printf("Psycho acoustic problems.\n");
            return ESP_FAIL;
        }
        else
        {
            printf("The conversion was not successful: %d.\n", num_samples_encoded);
            return ESP_FAIL;
        }


        // printf("Contents of mp3buffer = ");
        /*   for(int i = 0; i < num_samples_encoded; i++) {
        printf("%02X", mp3buf[i]);
        }
        */
        bytes_written = (int)fwrite(encoder->mp3buf, 1, num_samples_encoded, f);
        if (bytes_written != num_samples_encoded)
        {
            printf("Error writing to MP3 file\n");
            return ESP_FAIL;
        }

        encoder->frames++;
    }

    printf("\n");

    gettimeofday (&tvalAfter, NULL);

    printf("Fist Frame time in microseconds: %ld microseconds\n",
            ((tvalFirstFrame.tv_sec - tvalBefore.tv_sec)*1000000L
            +tvalFirstFrame.tv_usec) - tvalBefore.tv_usec
            );


    printf("Total time in microseconds: %ld microseconds\n",
            ((tvalAfter.tv_sec - tvalBefore.tv_sec)*1000000L
            +tvalAfter.tv_usec) - tvalBefore.tv_usec
            );

    printf ("Total frames: %d TotalBytes: %d\n", encoder->frames, encoder->total);


    num_samples_encoded = lame_encode_flush(encoder->lame, encoder->mp3buf, encoder->mp3buf_size);
    if(num_samples_encoded < 0)
    {
        if(num_samples_encoded == -1)
        {
            printf("mp3buffer is probably not big enough.\n");
        }
        else
        {
            printf("MP3 internal error.\n");
        }
        return ESP_FAIL;
    }
    else
    {
        for(int i = 0; i < num_samples_encoded; i++)
        {
            printf("%02X ", encoder->mp3buf[i]);
        }

        encoder->total += num_samples_encoded;
        printf("Flushing stage yielded %d frames.\n", num_samples_encoded);
        bytes_written = (int)fwrite(encoder->mp3buf, 1, num_samples_encoded, f);

        if (bytes_written != num_samples_encoded)
        {
            printf("Error writing to MP3 file\n");
            return ESP_FAIL;
        }
    }

    // =========================================================
    lame_close(encoder->lame);
    printf("\nClose\n");

    return ESP_OK;
}

short int sample_queue_buffer[1152 * 3];


static intr_handle_t s_timer_handle;

void init_timer(int timer_period_us)
{
    timer_config_t config = {
            .alarm_en = true,
            .counter_en = false,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = true,
            .divider = 80   /* 1 us per tick */
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_period_us);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_isr, NULL, 0, &s_timer_handle);

    timer_start(TIMER_GROUP_0, TIMER_0);
}

void lameTest()
{
//    uint64_t last_timer_tick_count = 0;
//
//    while (1)
//    {
//        uint64_t current_ticks = timer_ticks;
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//
//        if (last_timer_tick_count > 0)
//        {
//            printf("%llu timer ticks elapsed\n", current_ticks - last_timer_tick_count);
//        }
//
//        last_timer_tick_count = current_ticks;
//    }
//
//    while (1) vTaskDelay(500 / portTICK_RATE_MS);

    mcp3201_begin(0, 0, 0, 0, 0);

    sample_queue_handle = xQueueCreateStatic(1200, sizeof(sample_queue_buffer[0]), (uint8_t *)sample_queue_buffer, &sample_queue);
    if (!sample_queue_handle) {
        printf("Unable to create sample queue.\n");
        return;
    }

     esp_err_t ret = mount_sd_card("/sdcard");
     if (ret != ESP_OK) {
         printf("Error %d mounting SD card\n", ret);
         return;
     }

     FILE *f = fopen("/sdcard/SAMPLE.MP3", "w");
     if (f == NULL) {
         printf("Error creating MP3 file: %d\n", ret);
         unmount_sd_card();
         return;
     }

     struct esp_encoder encoder;
     ret = esp_encoder__initialize(&encoder, 8000, 1);
     if (f == NULL) {
         printf("Error initializing MP3 encoder: %d\n", ret);
         fclose(f);
         unmount_sd_card();
         return;
     }

     // Everything is ready. Start the data sampler timer.
     init_timer(125);

     ret = esp_encoder__encode_audio(&encoder, f);
     if (f == NULL) {
         printf("Error encoding MP3 to file: %d\n", ret);
         fclose(f);
         unmount_sd_card();
         return;
     }

     printf("%u lost samples\n", lost_samples);

     fclose(f);
     unmount_sd_card();
}
