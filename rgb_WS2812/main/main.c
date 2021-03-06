/*  ----------------------------------------------------------------------------
    File: main.c
    Author(s):  Lucas Bruder <LBruder@me.com>
    Date Created: 11/23/2016
    Last modified: 11/26/2016

    ------------------------------------------------------------------------- */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_task.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "freertos/semphr.h"

#include <stdio.h>

#define LED_STRIP_LENGTH 1U
#define LED_STRIP_RMT_INTR_NUM 19U

static struct led_color_t led_strip_buf_1[LED_STRIP_LENGTH];
static struct led_color_t led_strip_buf_2[LED_STRIP_LENGTH];

void app_main(void)
{
    struct led_strip_t led_strip = {
        .rgb_led_type = RGB_LED_TYPE_WS2812,
        .rmt_channel = RMT_CHANNEL_1,
        .rmt_interrupt_num = LED_STRIP_RMT_INTR_NUM,
        .gpio = GPIO_NUM_5,
        .led_strip_buf_1 = led_strip_buf_1,
        .led_strip_buf_2 = led_strip_buf_2,
        .led_strip_length = LED_STRIP_LENGTH};
    led_strip.access_semaphore = xSemaphoreCreateBinary();

    bool led_init_ok = led_strip_init(&led_strip);
    assert(led_init_ok);

    struct led_color_t led_color = {
        .red = 0,
        .green = 0,
        .blue = 0,
    };

    while (true)
    {
        led_color.red = 75;
        led_color.green = 0;
        led_color.blue = 0;

        led_strip_set_pixel_color(&led_strip, 0, &led_color);    
        led_strip_show(&led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        led_color.red = 0;
        led_color.green = 75;
        led_color.blue = 0;

        led_strip_set_pixel_color(&led_strip, 0, &led_color);    
        led_strip_show(&led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        led_color.red = 0;
        led_color.green = 0;
        led_color.blue = 75;

        led_strip_set_pixel_color(&led_strip, 0, &led_color);    
        led_strip_show(&led_strip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
