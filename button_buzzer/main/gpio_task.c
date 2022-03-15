/* Copyright (c) 2017 pcbreflux. All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 */
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "driver/ledc.h"

#include "sdkconfig.h"

#include "gpio_task.h"

#define c 261
#define d 294
#define e 329
#define f 349
#define g 391
#define gS 415
#define a 440
#define aS 455
#define b 466
#define cH 523
#define cSH 554
#define dH 587
#define dSH 622
#define eH 659
#define fH 698
#define fSH 740
#define gH 784
#define gSH 830
#define aH 880
#define aSH 932

#define GPIO_INPUT 4
#define GPIO_OUTPUT 23
#define GPIO_OUTPUT_SPEED LEDC_LOW_SPEED_MODE // back too old git commit :-(
// #define GPIO_OUTPUT_SPEED LEDC_HIGH_SPEED_MODE

#define TAG "BUZZER"

EventGroupHandle_t alarm_eventgroup;

const int GPIO_SENSE_BIT = BIT0;

void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    BaseType_t xHigherPriorityTaskWoken;
    if (gpio_num == GPIO_INPUT)
    {
        xEventGroupSetBitsFromISR(alarm_eventgroup, GPIO_SENSE_BIT, &xHigherPriorityTaskWoken);
    }
}

void init_gpio(void)
{
    gpio_config_t io_conf;

    // interrupt of falling edge
    io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1 << GPIO_INPUT);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    // install gpio isr service
    gpio_install_isr_service(0); // no flags
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT, gpio_isr_handler, (void *)GPIO_INPUT);
}

void sound(int gpio_num, uint32_t freq, uint32_t duration)
{

    // Configure timer zero
    ledc_timer_config_t ledc_timer =
        {
            .duty_resolution = LEDC_TIMER_10_BIT,
            .freq_hz = freq,
            .speed_mode = GPIO_OUTPUT_SPEED,
            .timer_num = LEDC_TIMER_0};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel =
        {
            .channel = LEDC_CHANNEL_0,
            .duty = 0x0,
            .hpoint = 0,
            .gpio_num = gpio_num,
            .intr_type = LEDC_INTR_DISABLE,
            .speed_mode = GPIO_OUTPUT_SPEED,
            .timer_sel = LEDC_TIMER_0,
        };
    ledc_channel_config(&ledc_channel);

    // ledc_channel_config_t ledc_conf;
    // ledc_conf.gpio_num   = gpio_num;
    // ledc_conf.speed_mode = GPIO_OUTPUT_SPEED;
    // ledc_conf.channel    = LEDC_CHANNEL_0;
    // ledc_conf.intr_type  = LEDC_INTR_DISABLE;
    // ledc_conf.timer_sel  = LEDC_TIMER_0;
    // ledc_conf.duty       = 0x01FF; // 50%=0x3FFF, 100%=0x7FFF for 15 Bit
    //                          // 50%=0x01FF, 100%=0x03FF for 10 Bit
    // ledc_channel_config(&ledc_conf);

    // start
    ledc_set_duty(GPIO_OUTPUT_SPEED, LEDC_CHANNEL_0, 0x7F); // 12% duty - play here for your speaker or buzzer
    ledc_update_duty(GPIO_OUTPUT_SPEED, LEDC_CHANNEL_0);
    vTaskDelay(duration / portTICK_PERIOD_MS);
    // stop
    ledc_set_duty(GPIO_OUTPUT_SPEED, LEDC_CHANNEL_0, 0);
    ledc_update_duty(GPIO_OUTPUT_SPEED, LEDC_CHANNEL_0);
}

void naagin()
{
    sound(GPIO_OUTPUT, eH, 200);
    sound(GPIO_OUTPUT, dH, 200);
    sound(GPIO_OUTPUT, fH, 200);
    sound(GPIO_OUTPUT, dH, 200);
    sound(GPIO_OUTPUT, eH, 200);
    sound(GPIO_OUTPUT, cH, 200);
    sound(GPIO_OUTPUT, dH, 200);
    sound(GPIO_OUTPUT, cH, 200);

    sound(GPIO_OUTPUT, eH, 200);
    sound(GPIO_OUTPUT, dH, 200);
    sound(GPIO_OUTPUT, fH, 200);
    sound(GPIO_OUTPUT, dH, 200);
    sound(GPIO_OUTPUT, eH, 200);
    sound(GPIO_OUTPUT, cH, 200);
    sound(GPIO_OUTPUT, dH, 200);

    sound(GPIO_OUTPUT, eH, 200);
    sound(GPIO_OUTPUT, fH, 200);
    sound(GPIO_OUTPUT, aH, 500);

    sound(GPIO_OUTPUT, aSH, 200);
    sound(GPIO_OUTPUT, aSH, 200);
    sound(GPIO_OUTPUT, aH, 500);

    sound(GPIO_OUTPUT, aSH, 200);
    sound(GPIO_OUTPUT, aSH, 200);
    sound(GPIO_OUTPUT, aH, 500);

    sound(GPIO_OUTPUT, aSH, 200);
    sound(GPIO_OUTPUT, aSH, 200);
    sound(GPIO_OUTPUT, aH, 500);

    sound(GPIO_OUTPUT, eH, 200);
    sound(GPIO_OUTPUT, fH, 200);
    sound(GPIO_OUTPUT, gH, 500);

    sound(GPIO_OUTPUT, aH, 200);
    sound(GPIO_OUTPUT, aH, 200);
    sound(GPIO_OUTPUT, gH, 500);

    sound(GPIO_OUTPUT, aH, 200);
    sound(GPIO_OUTPUT, aH, 200);
    sound(GPIO_OUTPUT, gH, 500);

    sound(GPIO_OUTPUT, fH, 200);
    sound(GPIO_OUTPUT, eH, 200);
    sound(GPIO_OUTPUT, dH, 200);
}

// based on http://processors.wiki.ti.com/index.php/Playing_The_Imperial_March#Code
// original composed by John Williams for the film Star Wars: The Empire Strikes Back
void play_march(uint8_t longplay)
{

    sound(GPIO_OUTPUT, a, 500);
    sound(GPIO_OUTPUT, a, 500);
    sound(GPIO_OUTPUT, a, 500);
    sound(GPIO_OUTPUT, f, 350);
    sound(GPIO_OUTPUT, cH, 150);
    sound(GPIO_OUTPUT, a, 500);
    sound(GPIO_OUTPUT, f, 350);
    sound(GPIO_OUTPUT, cH, 150);
    sound(GPIO_OUTPUT, a, 650);

    vTaskDelay(150 / portTICK_PERIOD_MS);
    // end of first bit

    sound(GPIO_OUTPUT, eH, 500);
    sound(GPIO_OUTPUT, eH, 500);
    sound(GPIO_OUTPUT, eH, 500);
    sound(GPIO_OUTPUT, fH, 350);
    sound(GPIO_OUTPUT, cH, 150);
    sound(GPIO_OUTPUT, gS, 500);
    sound(GPIO_OUTPUT, f, 350);
    sound(GPIO_OUTPUT, cH, 150);
    sound(GPIO_OUTPUT, a, 650);

    vTaskDelay(150 / portTICK_PERIOD_MS);
    // end of second bit...

    sound(GPIO_OUTPUT, aH, 500);
    sound(GPIO_OUTPUT, a, 300);
    sound(GPIO_OUTPUT, a, 150);
    sound(GPIO_OUTPUT, aH, 400);
    sound(GPIO_OUTPUT, gSH, 200);
    sound(GPIO_OUTPUT, gH, 200);
    sound(GPIO_OUTPUT, fSH, 125);
    sound(GPIO_OUTPUT, fH, 125);
    sound(GPIO_OUTPUT, fSH, 250);

    vTaskDelay(250 / portTICK_PERIOD_MS);

    sound(GPIO_OUTPUT, aS, 250);
    sound(GPIO_OUTPUT, dSH, 400);
    sound(GPIO_OUTPUT, dH, 200);
    sound(GPIO_OUTPUT, cSH, 200);
    sound(GPIO_OUTPUT, cH, 125);
    sound(GPIO_OUTPUT, b, 125);
    sound(GPIO_OUTPUT, cH, 250);

    vTaskDelay(250 / portTICK_PERIOD_MS);

    sound(GPIO_OUTPUT, f, 125);
    sound(GPIO_OUTPUT, gS, 500);
    sound(GPIO_OUTPUT, f, 375);
    sound(GPIO_OUTPUT, a, 125);
    sound(GPIO_OUTPUT, cH, 500);
    sound(GPIO_OUTPUT, a, 375);
    sound(GPIO_OUTPUT, cH, 125);
    sound(GPIO_OUTPUT, eH, 650);

    // end of third bit... (Though it doesn't play well)
    // let's repeat it
    if (longplay >= 1)
    {
        sound(GPIO_OUTPUT, aH, 500);
        sound(GPIO_OUTPUT, a, 300);
        sound(GPIO_OUTPUT, a, 150);
        sound(GPIO_OUTPUT, aH, 400);
        sound(GPIO_OUTPUT, gSH, 200);
        sound(GPIO_OUTPUT, gH, 200);
        sound(GPIO_OUTPUT, fSH, 125);
        sound(GPIO_OUTPUT, fH, 125);
        sound(GPIO_OUTPUT, fSH, 250);

        vTaskDelay(250 / portTICK_PERIOD_MS);

        sound(GPIO_OUTPUT, aS, 250);
        sound(GPIO_OUTPUT, dSH, 400);
        sound(GPIO_OUTPUT, dH, 200);
        sound(GPIO_OUTPUT, cSH, 200);
        sound(GPIO_OUTPUT, cH, 125);
        sound(GPIO_OUTPUT, b, 125);
        sound(GPIO_OUTPUT, cH, 250);

        vTaskDelay(250 / portTICK_PERIOD_MS);

        sound(GPIO_OUTPUT, f, 250);
        sound(GPIO_OUTPUT, gS, 500);
        sound(GPIO_OUTPUT, f, 375);
        sound(GPIO_OUTPUT, cH, 125);
        sound(GPIO_OUTPUT, a, 500);
        sound(GPIO_OUTPUT, f, 375);
        sound(GPIO_OUTPUT, cH, 125);
        sound(GPIO_OUTPUT, a, 650);
        // end of the song
    }
}

void beep()
{
    sound(GPIO_OUTPUT, eH, 300);
}

// Task for single, double, triple tap and long and short press
void gpio_task(void *pvParameters)
{
    EventBits_t bits;
    alarm_eventgroup = xEventGroupCreate();
    init_gpio();

    long int time_current = 0;
    long int time_released = 0;
    long int time_pressed = 0;
    long int current_time2 = 0;

    int FLAG = 0;
    int button_state_ = 0;
    int button_press_count = 0;

    while (1)
    {
        bits = gpio_get_level(GPIO_INPUT);

        if (bits == 1 && FLAG == 0)
        {
            time_released = esp_timer_get_time();
            // printf("BUTTON STATE : %d\n", bits);
            FLAG = 1;
            button_state_ = 1;
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        else if (bits == 0 && FLAG == 1)
        {
            time_pressed = esp_timer_get_time();
            // printf("BUTTON STATE : %d\n", bits);
            FLAG = 0;
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        time_current = esp_timer_get_time();

        if ((time_released - time_pressed <= 500000) && button_state_ == 1)
        {
            current_time2 = esp_timer_get_time();
            if (button_press_count == 0)
            {
                // printf("SINGLE TAP\n");
                button_press_count = 1;
            }
            else if (button_press_count == 1)
            {
                // printf("DOUBLE TAP\n");
                button_press_count = 2;
            }
            else if (button_press_count == 2)
            {
                // printf("TRIPLE TAP\n");
                button_press_count = 3;
            }
            button_state_ = 0;
        }
        else if ((time_released - time_pressed >= 1000000) && (time_released - time_pressed <= 3000000) && button_state_ == 1)
        {
            printf("SHORT PRESS\n");
            button_state_ = 0;
        }
        else if ((time_released - time_pressed > 3000000) && button_state_ == 1)
        {
            printf("LONG PRESS\n");
            button_state_ = 0;
        }

        if (time_current - current_time2 > 1000000 && button_press_count > 0)
        {
            printf("TAP : %d\n", button_press_count);
            button_press_count = 0;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// Task for Button Press
// void gpio_task(void *pvParameters) {
// 	EventBits_t bits;
// 	alarm_eventgroup = xEventGroupCreate();
//     init_gpio();
// 	while (1) {
// 		bits=xEventGroupWaitBits(alarm_eventgroup, GPIO_SENSE_BIT,pdTRUE, pdFALSE, 60000 / portTICK_RATE_MS); // max wait 60s
// 		if(bits==1) {
//             count++;
//             vTaskDelay(250/portTICK_PERIOD_MS);
// 			xEventGroupClearBits(alarm_eventgroup, GPIO_SENSE_BIT);
//             ESP_LOGI(TAG, "Count: %d", count);
//             // beep();
//             naagin();
// 			// play_march(0);
//         }
//     }
// 	ESP_LOGI(TAG, "All done!");
// 	vTaskDelete(NULL);
// }