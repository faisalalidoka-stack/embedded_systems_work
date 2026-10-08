#include "ui_controller.h"

#include <stdio.h>
#include "lcd_i2c.h"
#include "temp_monitor.h"
#include "freertos/task.h"

#define REFRESH_MS       1000
#define LED_ON_MS        3000
#define LED_OFF_MSG_MS   1500
#define LCD_DEGREE_CHAR  0xDF

static ui_config_t s_cfg;

static void show_temperature(void)
{
    temp_sample_t s;
    char line[17];

    if (temp_monitor_get_latest(&s) != ESP_OK) {
        lcd_print_line(0, "Temperature:");
        lcd_print_line(1, "Reading...");
        return;
    }

    if (s.status != ESP_OK) {
        lcd_print_line(0, "Temperature:");
        lcd_print_line(1, "Sensor error");
        return;
    }

    snprintf(line, sizeof(line), "%.2f%cC", s.celsius, LCD_DEGREE_CHAR);
    lcd_print_line(0, "Temperature:");
    lcd_print_line(1, line);
}

static void led_cycle(gpio_num_t led, const char *on_msg, const char *off_msg)
{
    gpio_set_level(led, 1);
    lcd_print_line(0, on_msg);
    lcd_print_line(1, "");
    vTaskDelay(pdMS_TO_TICKS(LED_ON_MS));

    gpio_set_level(led, 0);
    lcd_print_line(0, off_msg);
    vTaskDelay(pdMS_TO_TICKS(LED_OFF_MSG_MS));

    xQueueReset(s_cfg.key_queue);   /* drop presses made during the cycle */
}

static void ui_task(void *arg)
{
    show_temperature();

    for (;;) {
        char key;

        if (xQueueReceive(s_cfg.key_queue, &key, pdMS_TO_TICKS(REFRESH_MS)) == pdTRUE) {
            if (key == '5') {
                led_cycle(s_cfg.blue_gpio, "BLUE LED ON", "BLUE LED OFF");
            } else if (key == 'D') {
                led_cycle(s_cfg.red_gpio, "RED LED ON", "RED LED OFF");
            }
        }

        show_temperature();
    }
}

esp_err_t ui_controller_start(const ui_config_t *cfg)
{
    if (cfg == NULL || cfg->key_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_cfg = *cfg;

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << s_cfg.blue_gpio) | (1ULL << s_cfg.red_gpio),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io);
    if (err != ESP_OK) {
        return err;
    }
    gpio_set_level(s_cfg.blue_gpio, 0);
    gpio_set_level(s_cfg.red_gpio, 0);

    return (xTaskCreate(ui_task, "ui", 3072, NULL, 3, NULL) == pdPASS)
               ? ESP_OK : ESP_ERR_NO_MEM;
}