#pragma once

#include <stdint.h>
#include "esp_err.h"

/* 16x2 HD44780 behind a PCF8574 I2C backpack (4-bit mode). */
esp_err_t lcd_init(int sda_gpio, int scl_gpio, uint8_t i2c_addr);
void      lcd_clear(void);
void      lcd_set_cursor(uint8_t col, uint8_t row);
void      lcd_print(const char *text);

/* Writes text to a row, truncated/padded to 16 characters (no flicker, no clear needed). */
void      lcd_print_line(uint8_t row, const char *text);