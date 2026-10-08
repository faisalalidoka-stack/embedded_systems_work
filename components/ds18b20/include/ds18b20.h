#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

/* 12-bit conversion takes up to 750 ms; 50 ms margin covers FreeRTOS tick rounding. */
#define DS18B20_CONVERSION_MS  800

/* Returns ESP_ERR_NOT_FOUND if no presence pulse is seen (check wiring / pull-up). */
esp_err_t ds18b20_init(gpio_num_t pin);

/* Kicks off a conversion and returns immediately (takes ~2 ms). */
esp_err_t ds18b20_start_conversion(void);

/* Call at least DS18B20_CONVERSION_MS after start_conversion.
 * Errors: ESP_ERR_NOT_FOUND (no presence), ESP_ERR_INVALID_RESPONSE (bus stuck high/low,
 * sensor not talking), ESP_ERR_INVALID_CRC (corrupted data, usually timing). */
esp_err_t ds18b20_read_temperature(float *out_celsius);