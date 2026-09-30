#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t ow_init(gpio_num_t pin);
bool      ow_reset(void);               /* true if a device answered with a presence pulse */
void      ow_write_byte(uint8_t byte);
uint8_t   ow_read_byte(void);
uint8_t   ow_crc8(const uint8_t *data, uint8_t len);