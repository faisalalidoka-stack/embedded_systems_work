#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "keypad.h"
#include "lcd_i2c.h"
#include "temp_monitor.h"
#include "ui_controller.h"

#define TAG "main"

#define SENSOR_GPIO       4
#define SAMPLE_PERIOD_MS  1000

#define LCD_SDA_GPIO      21
#define LCD_SCL_GPIO      22
#define LCD_I2C_ADDR      0x27

#define BLUE_LED_GPIO     23
#define RED_LED_GPIO      27

void app_main(void)
{
    /* LCD */
    if (lcd_init(LCD_SDA_GPIO, LCD_SCL_GPIO, LCD_I2C_ADDR) != ESP_OK) {
        ESP_LOGE(TAG, "LCD init failed");
        return;
    }
    lcd_print_line(0, "Starting...");
    lcd_print_line(1, "");

    /* Temperature telemetry (own tasks, never blocked by the UI) */
    temp_monitor_config_t tcfg = {
        .gpio_num  = SENSOR_GPIO,
        .period_ms = SAMPLE_PERIOD_MS,
    };
    if (temp_monitor_start(&tcfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start temperature monitor");
        lcd_print_line(0, "Monitor failed");
        return;
    }

    /* Keypad ISR -> task -> queue of key characters */
    QueueHandle_t key_queue = xQueueCreate(8, sizeof(char));
    if (key_queue == NULL) {
        ESP_LOGE(TAG, "Key queue alloc failed");
        return;
    }

    keypad_config_t kcfg = {
        .rows = { GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5,  GPIO_NUM_17 },
        .cols = { GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_25, GPIO_NUM_26 },
    };
    if (keypad_init(&kcfg, key_queue) != ESP_OK) {
        ESP_LOGE(TAG, "Keypad init failed");
        return;
    }

    /* UI: consumes keys, drives LEDs and the LCD */
    ui_config_t ucfg = {
        .key_queue = key_queue,
        .blue_gpio = BLUE_LED_GPIO,
        .red_gpio  = RED_LED_GPIO,
    };
    if (ui_controller_start(&ucfg) != ESP_OK) {
        ESP_LOGE(TAG, "UI start failed");
        return;
    }

    ESP_LOGI(TAG, "System running; app_main returning");
}