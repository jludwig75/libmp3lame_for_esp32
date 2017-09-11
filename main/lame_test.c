#include "lame.h"
#include "lame_test.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

extern const uint8_t Sample16kHz_raw_start[] asm("_binary_Sample16kHz_mono_8kHz_raw_start");
extern const uint8_t Sample16kHz_raw_end[]   asm("_binary_Sample16kHz_mono_8kHz_raw_end");

void lameTest()
{
 lame_t lame;
 unsigned int sampleRate = 8000;
 short int *pcm_samples, *pcm_samples_end;
 int framesize = 0;
 int num_samples_encoded = 0, total=0, frames=0;
 size_t free8start, free32start;
 int nsamples=1152;
 unsigned char *mp3buf;
 const int mp3buf_size=2000;  //mp3buf_size in bytes = 1.25*num_samples + 7200
 struct timeval tvalBefore, tvalFirstFrame, tvalAfter;
 int bytes_written;
 int num_channels = 1;

 esp_err_t ret = mount_sd_card("/sdcard");
 if (ret != ESP_OK) {
	 printf("Error %d mounting SD card\n", ret);
	 return;
 }

 FILE *f = fopen("/sdcard/SAMPLE.MP3", "w");
 if (f == NULL) {
	 printf("Error creating MP3 file\n");
     unmount_sd_card();
     return;
 }

 free8start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_8BIT);
 free32start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_32BIT);
 printf("pre lame_init() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

 mp3buf=malloc(mp3buf_size);

 /* Init lame flags.*/
 lame = lame_init();

 free8start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_8BIT);
 free32start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_32BIT);
 printf("post lame_init() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

 if(!lame) {
   printf("Unable to initialize lame object.\n");
   fclose(f);
   unmount_sd_card();
   return;
  } else {
   printf("Able to initialize lame object.\n");
  }

 /* set other parameters.*/
 lame_set_VBR(lame, vbr_default);
 lame_set_num_channels(lame, num_channels);
 lame_set_in_samplerate(lame, sampleRate);
 lame_set_quality(lame, 7);  /* set for high speed and good quality. */
 lame_set_mode(lame, num_channels == 1 ? MONO : JOINT_STEREO);  /* audio is joint stereo */

// lame_set_out_samplerate(lame, sampleRate);
 printf("Able to set a number of parameters too.\n");


 /* set more internal variables. check for failure.*/
 int initRet = lame_init_params(lame);
 if(initRet < 0) printf("Fail in setting internal parameters with code=%d\n",initRet);
 else printf("OK setting internal parameters\n");


 free8start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_8BIT);
 free32start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_32BIT);
 printf("post lame_init_params() free mem8bit: %d mem32bit: %d\n",free8start,free32start);

// lame_print_config(lame);
// lame_print_internals(lame);

 framesize = lame_get_framesize(lame);
 printf("Framesize = %d\n", framesize);
// assert(framesize <= 1152);

 pcm_samples = (short int *)Sample16kHz_raw_start;
 pcm_samples_end = (short int *)Sample16kHz_raw_end;


 gettimeofday (&tvalBefore, NULL);
 while ( pcm_samples_end - pcm_samples > 0)
 //for (int j=0;j<1;j++)
 {
	   //  printf("\n=============== lame_encode_buffer_interleaved================ \n");
 /* encode samples. */

	 if (num_channels == 1) {
		 // Mono
		 num_samples_encoded = lame_encode_buffer(lame, pcm_samples, pcm_samples, nsamples, mp3buf, mp3buf_size);
	 }
	 else {
		 // Stereo
	     // Pass in the number of samples in one channel. That is the total number of samples divided by num_channels.
		 num_samples_encoded = lame_encode_buffer_interleaved(lame, pcm_samples, nsamples / num_channels, mp3buf, mp3buf_size);
	 }

  //   printf("number of samples encoded = %d pcm_samples %p \n", num_samples_encoded, pcm_samples);

	  if (total==0) gettimeofday (&tvalFirstFrame, NULL);

     /* check for value returned.*/
     if(num_samples_encoded > 1) {
       //printf("It seems the conversion was successful.\n");
    	 total+=num_samples_encoded;
     } else if(num_samples_encoded == -1) {
       printf("mp3buf was too small\n");
       fclose(f);
       unmount_sd_card();
       return ;
     } else if(num_samples_encoded == -2) {
       printf("There was a malloc problem.\n");
       fclose(f);
       unmount_sd_card();
       return ;
     } else if(num_samples_encoded == -3) {
       printf("lame_init_params() not called.\n");
       fclose(f);
       unmount_sd_card();
       return ;
     } else if(num_samples_encoded == -4) {
       printf("Psycho acoustic problems.\n");
       fclose(f);
       unmount_sd_card();
       return ;
     } else {
       printf("The conversion was not successful.\n");
       fclose(f);
       unmount_sd_card();
       return ;
     }


    // printf("Contents of mp3buffer = ");
  /*   for(int i = 0; i < num_samples_encoded; i++) {
    	 printf("%02X", mp3buf[i]);
     }
*/
     bytes_written = (int)fwrite(mp3buf, 1, num_samples_encoded, f);
     if (bytes_written != num_samples_encoded) {
    	 printf("Error writing to MP3 file\n");
    	 fclose(f);
         unmount_sd_card();
         return ;
     }

    printf(".");
    pcm_samples += nsamples;
    frames++;

#if 0
    // to test infinite loop
    if ( pcm_samples_end - pcm_samples <= 0){
    	pcm_samples = (short int *)Sample16kHz_raw_start;
    	free8start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_8BIT);
    	free32start=xPortGetFreeHeapSizeCaps(MALLOC_CAP_32BIT);
    	printf("LOOP: free mem8bit: %d mem32bit: %d frames encoded: %d bytes:%d\n",free8start,free32start,frames,total);
    }
#endif
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

 printf ("Total frames: %d TotalBytes: %d\n", frames, total);


 num_samples_encoded = lame_encode_flush(lame, mp3buf, mp3buf_size);
 if(num_samples_encoded < 0) {
   if(num_samples_encoded == -1) {
     printf("mp3buffer is probably not big enough.\n");
   } else {
     printf("MP3 internal error.\n");
   }
   unmount_sd_card();
   return ;
 } else {
     for(int i = 0; i < num_samples_encoded; i++) {
       printf("%02X ", mp3buf[i]);
     }
     total += num_samples_encoded;
     printf("Flushing stage yielded %d frames.\n", num_samples_encoded);
     bytes_written = (int)fwrite(mp3buf, 1, num_samples_encoded, f);
     if (bytes_written != num_samples_encoded) {
    	 printf("Error writing to MP3 file\n");
    	 fclose(f);
         unmount_sd_card();
         return;
     }
 }

 fclose(f);

// =========================================================

 lame_close(lame);
 printf("\nClose\n");
 unmount_sd_card();

 while (1) vTaskDelay(500 / portTICK_RATE_MS);

 return;
}
