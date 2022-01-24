/**
 * ESP-32 IDF library for control TM1637 LED 7-Segment display
 *
 * Author: Petro <petro@petro.ws>
 *
 * Project homepage: https://github.com/petrows/esp-32-tm1637
 * Example: https://github.com/petrows/esp-32-tm1637-example
 *
 * License: MIT (see LICENSE file included)
 *
 */

#ifndef TM1637_H
#define TM1637_H

#include <inttypes.h>
#include <stdbool.h>
#include <driver/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tm;

typedef struct {
	gpio_num_t m_pin_clk;
	gpio_num_t m_pin_dta;
	uint8_t m_brightness;
} tm1637_led_t;

/**
 * @brief Constructs new LED TM1637 object
 *
 * @param pin_clk GPIO pin for CLK input of LED module
 * @param pin_data GPIO pin for DIO input of LED module
 * @return
 */
tm1637_led_t * tm1637_init(gpio_num_t pin_clk, gpio_num_t pin_data);

/**
 * @brief Set brightness level. Note - will be set after next display render
 * @param led LED object
 * @param level Brightness level 0..7 value
 */
void tm1637_set_brightness(tm1637_led_t * led, uint8_t level);

/**
 * @brief Set one-segment number, also controls dot of this segment
 * @param led LED object
 * @param segment_idx Segment index (0..3)
 * @param num Number to set (0x00..0x0F, 0xFF for clear)
 * @param dot Display dot of this segment
 */
void tm1637_set_segment_number(tm1637_led_t * led, const uint8_t segment_idx, const uint8_t num, const bool dot);

/**
 * @brief Set one-segment raw segment data
 * @param led LED object
 * @param segment_idx Segment index (0..3)
 * @param data Raw data, bitmask is XGFEDCBA
 */
void tm1637_set_segment_raw(tm1637_led_t * led, const uint8_t segment_idx, const uint8_t data);

/**
 * @brief Set full display number, in decimal encoding
 * @param led LED object
 * @param number Display number (0...9999)
 */
void tm1637_set_number(tm1637_led_t * led, uint16_t number);

/**
 * @brief Set full display number, in decimal encoding + control leading zero
 * @param led LED object
 * @param number Display number (0...9999)
 * @param lead_zero Display leading zero(s)
 */
void tm1637_set_number_lead(tm1637_led_t * led, uint16_t number, const bool lead_zero);

/**
 * @brief Set full display number, in decimal encoding + control leading zero + control dot display
 * @param led LED object
 * @param number Display number (0...9999)
 * @param lead_zero Display leading zero(s)
 * @param dot_mask Dot mask, bits left-to-right
 */
void tm1637_set_number_lead_dot(tm1637_led_t * led, uint16_t number, const bool lead_zero, const uint8_t dot_mask);

/**
 * @brief Set floating point number, correctly handling negative numbers
 * @param led LED object
 * @param, n Floating point number
 */
void tm1637_set_float(tm1637_led_t * led, float n);

#ifdef __cplusplus
}
#endif

#endif // TM1637_H
