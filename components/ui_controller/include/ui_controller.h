#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    QueueHandle_t key_queue;   /* items are char */
    gpio_num_t    blue_gpio;
    gpio_num_t    red_gpio;
} ui_config_t;

esp_err_t ui_controller_start(const ui_config_t *cfg);