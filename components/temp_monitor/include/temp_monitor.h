#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

typedef struct {
    gpio_num_t gpio_num;
    uint32_t   period_ms;   /* must be > 750 ms */
} temp_monitor_config_t;

esp_err_t temp_monitor_start(const temp_monitor_config_t *cfg);