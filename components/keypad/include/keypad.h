#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    gpio_num_t rows[4];   /* open-drain outputs, idle LOW */
    gpio_num_t cols[4];   /* inputs, pull-up, falling-edge interrupt */
} keypad_config_t;

/* Every confirmed key press is sent to out_queue as a single char (queue item size = sizeof(char)). */
esp_err_t keypad_init(const keypad_config_t *cfg, QueueHandle_t out_queue);