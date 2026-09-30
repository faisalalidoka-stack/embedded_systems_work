#include "esp_log.h"
#include "temp_monitor.h"

#define TAG "main"

#define SENSOR_GPIO       4
#define SAMPLE_PERIOD_MS  2000

void app_main(void)
{
    temp_monitor_config_t cfg = {
        .gpio_num  = SENSOR_GPIO,
        .period_ms = SAMPLE_PERIOD_MS,
    };

    if (temp_monitor_start(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start temperature monitor");
        return;
    }

    ESP_LOGI(TAG, "Temperature monitor running; app_main returning");
}