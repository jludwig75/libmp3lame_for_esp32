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


#include "freertos/queue.h"

#include "esp_intr_alloc.h"
#include "esp_attr.h"
#include "driver/timer.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/uart.h"


uint64_t timer_ticks = 0;


static void timer_isr(void* arg)
{
    /* We have not woken a task at the start of the ISR. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;

    // your code, runs in the interrupt
    uint16_t val = (uint16_t)adc1_get_voltage(ADC1_CHANNEL_6);

    uart_write_bytes(UART_NUM_2, (const char *)&val, sizeof(val));

    if (xHigherPriorityTaskWoken)
    {
        /* Actual macro used here is port specific. */

        portYIELD_FROM_ISR();
    }

    timer_ticks++;
}


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


uint8_t start_sequence[] = { 0x55, 0xAA, 'S', 'T', 'A', 'R', 'T', 0x55, 0xAA };

void lameTest()
{
     uart_config_t uart_config = {
            .baud_rate = 256000,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
        };
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    printf("UART configured...\n");

    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db);

    printf("ADC configured. Starting timer...\n");

    printf("Start recording client on UART 2. Press enter when ready...\n");
    char read_buffer[128];
    gets(read_buffer);

    // Send the start sequence to the client on UART2
    uart_write_bytes(UART_NUM_2, (const char *)start_sequence, sizeof(start_sequence));

     // Everything is ready. Start the data sampler timer.
    init_timer(125);

    while (1) vTaskDelay(500 / portTICK_RATE_MS);
}
