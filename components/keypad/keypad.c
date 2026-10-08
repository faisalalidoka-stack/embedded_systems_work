#include "keypad.h"

#include "esp_attr.h"
#include "esp_rom_sys.h"
#include "freertos/task.h"

#define DEBOUNCE_MS 20

static const char KEYMAP[4][4] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' },
};

static keypad_config_t s_cfg;
static QueueHandle_t   s_out;
static TaskHandle_t    s_task;

/* ISR: do nothing but wake the keypad task. */
static void IRAM_ATTR keypad_isr(void *arg)
{
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_task, &woken);
    if (woken) {
        portYIELD_FROM_ISR();
    }
}

static void rows_all_low(void)
{
    for (int i = 0; i < 4; i++) {
        gpio_set_level(s_cfg.rows[i], 0);
    }
}

static char scan(void)
{
    char key = 0;

    for (int r = 0; r < 4 && !key; r++) {
        for (int i = 0; i < 4; i++) {
            gpio_set_level(s_cfg.rows[i], (i == r) ? 0 : 1);   /* 1 = released (open-drain) */
        }
        esp_rom_delay_us(10);

        for (int c = 0; c < 4; c++) {
            if (gpio_get_level(s_cfg.cols[c]) == 0) {
                key = KEYMAP[r][c];
                break;
            }
        }
    }

    rows_all_low();
    return key;
}

static bool any_col_low(void)
{
    for (int c = 0; c < 4; c++) {
        if (gpio_get_level(s_cfg.cols[c]) == 0) {
            return true;
        }
    }
    return false;
}

static void keypad_task(void *arg)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);     /* sleeps until a column edge */
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));

        char key = scan();
        if (key) {
            xQueueSend(s_out, &key, 0);

            while (any_col_low()) {                  /* wait for release, sleeping */
                vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            }
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        }

        ulTaskNotifyTake(pdTRUE, 0);                 /* discard bounce edges */
    }
}

esp_err_t keypad_init(const keypad_config_t *cfg, QueueHandle_t out_queue)
{
    if (cfg == NULL || out_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_cfg = *cfg;
    s_out = out_queue;

    for (int i = 0; i < 4; i++) {
        gpio_config_t row = {
            .pin_bit_mask = 1ULL << s_cfg.rows[i],
            .mode         = GPIO_MODE_OUTPUT_OD,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        esp_err_t err = gpio_config(&row);
        if (err != ESP_OK) {
            return err;
        }
    }
    rows_all_low();

    if (xTaskCreate(keypad_task, "keypad", 2560, NULL, 6, &s_task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {   /* already installed is fine */
        return err;
    }

    for (int i = 0; i < 4; i++) {
        gpio_config_t col = {
            .pin_bit_mask = 1ULL << s_cfg.cols[i],
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_NEGEDGE,
        };
        err = gpio_config(&col);
        if (err != ESP_OK) {
            return err;
        }
        err = gpio_isr_handler_add(s_cfg.cols[i], keypad_isr, NULL);
        if (err != ESP_OK) {
            return err;
        }
    }

    return ESP_OK;
}