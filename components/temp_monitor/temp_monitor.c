#include "temp_monitor.h"
#include "ds18b20.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define TAG "temp_monitor"

typedef struct {
    int64_t timestamp_us;
    float   celsius;
    esp_err_t status;
} temp_sample_t;

static QueueHandle_t s_queue;
static TickType_t    s_period_ticks;

/* Producer: sleeps (no CPU use) during the 750 ms conversion and between samples. */
static void sensor_task(void *arg)
{
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        temp_sample_t s = { .celsius = 0.0f };

        s.status = ds18b20_start_conversion();
        if (s.status == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(DS18B20_CONVERSION_MS));   /* yields to other tasks */
            s.status = ds18b20_read_temperature(&s.celsius);
        }
        s.timestamp_us = esp_timer_get_time();

        if (xQueueSend(s_queue, &s, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Queue full, sample dropped");
        }

        vTaskDelayUntil(&last_wake, s_period_ticks);   /* fixed period, no drift */
    }
}

/* Consumer: blocks on the queue, only runs when a sample arrives. */
static void logger_task(void *arg)
{
    temp_sample_t s;

    for (;;) {
        if (xQueueReceive(s_queue, &s, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (s.status == ESP_OK) {
            ESP_LOGI(TAG, "[%lld ms] Temperature: %.2f C",
                     (long long)(s.timestamp_us / 1000), s.celsius);
        } else {
            ESP_LOGE(TAG, "[%lld ms] Read failed: %s",
                     (long long)(s.timestamp_us / 1000), esp_err_to_name(s.status));
        }
    }
}

esp_err_t temp_monitor_start(const temp_monitor_config_t *cfg)
{
    if (cfg == NULL || cfg->period_ms <= DS18B20_CONVERSION_MS) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ds18b20_init(cfg->gpio_num);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sensor init failed: %s (check wiring / pull-up)", esp_err_to_name(err));
        return err;
    }

    s_period_ticks = pdMS_TO_TICKS(cfg->period_ms);
    s_queue = xQueueCreate(8, sizeof(temp_sample_t));
    if (s_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(logger_task, "temp_log", 3072, NULL, 4, NULL) != pdPASS ||
        xTaskCreate(sensor_task, "temp_read", 3072, NULL, 5, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}