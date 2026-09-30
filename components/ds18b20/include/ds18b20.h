#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

/* Worst-case conversion time at the default 12-bit resolution. */
#define DS18B20_CONVERSION_MS  750

esp_err_t ds18b20_init(gpio_num_t pin);

/* Kicks off a conversion and returns immediately (takes ~1.5 ms). */
esp_err_t ds18b20_start_conversion(void);

/* Call at least DS18B20_CONVERSION_MS after start_conversion. */
esp_err_t ds18b20_read_temperature(float *out_celsius);