/**
 * @file app_main.c
 * @brief Example application for the TM1637 LED segment display
 */

#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "sdkconfig.h"
#include "tm1637.h"

#define TAG "app"

const gpio_num_t LED_CLK = CONFIG_TM1637_CLK_PIN;
const gpio_num_t LED_DTA = CONFIG_TM1637_DIO_PIN;

void app_main()
{
	tm1637_led_t * lcd = tm1637_init(LED_CLK, LED_DTA);
	// fill_display(lcd);
	// vTaskDelay(2000 / portTICK_RATE_MS);
	// conn_display(lcd);
	// vTaskDelay(2000 / portTICK_RATE_MS);
	// num_display( lcd,10);
	// vTaskDelay(2000 / portTICK_RATE_MS);
	// pass_display(lcd);
	// vTaskDelay(2000 / portTICK_RATE_MS);
	// fail_display(lcd);
	// vTaskDelay(2000 / portTICK_RATE_MS);
	// done_display(lcd);
	// vTaskDelay(2000 / portTICK_RATE_MS);
	// error_display(lcd);
	// vTaskDelay(2000 / portTICK_RATE_MS);
	// ack_display(lcd);
	while(1)
	{
		aliter_display(lcd);
		vTaskDelay(2000 / portTICK_RATE_MS);
		pass_display(lcd);
		vTaskDelay(2000 / portTICK_RATE_MS);
		fail_display(lcd);
		vTaskDelay(2000 / portTICK_RATE_MS);
	}
	
}

