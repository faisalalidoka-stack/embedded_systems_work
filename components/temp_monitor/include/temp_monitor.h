#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

typedef struct {
    gpio_num_t gpio_num;
    uint32_t   period_ms;   /* must be > 750 ms */
} temp_monitor_config_t;

typedef struct {
    int64_t   timestamp_us;
    float     celsius;
    esp_err_t status;
} temp_sample_t;

esp_err_t temp_monitor_start(const temp_monitor_config_t *cfg);

/* Copies the most recent sample. Returns ESP_ERR_NOT_FOUND if none yet. Never blocks. */
esp_err_t temp_monitor_get_latest(temp_sample_t *out);