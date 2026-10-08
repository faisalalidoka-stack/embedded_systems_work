#include "ds18b20.h"
#include "onewire.h"

#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "ds18b20"

#define CMD_SKIP_ROM         0xCC
#define CMD_CONVERT_T        0x44
#define CMD_READ_SCRATCHPAD  0xBE

#define READ_ATTEMPTS        3

esp_err_t ds18b20_init(gpio_num_t pin)
{
    esp_err_t err = ow_init(pin);
    if (err != ESP_OK) {
        return err;
    }
    return ow_reset() ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t ds18b20_start_conversion(void)
{
    if (!ow_reset()) {
        return ESP_ERR_NOT_FOUND;
    }
    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_CONVERT_T);
    return ESP_OK;
}

static esp_err_t read_scratchpad(uint8_t sp[9])
{
    if (!ow_reset()) {
        return ESP_ERR_NOT_FOUND;
    }
    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_READ_SCRATCHPAD);

    for (int i = 0; i < 9; i++) {
        sp[i] = ow_read_byte();
    }

    /* A floating bus reads all 1s; a bus stuck low reads all 0s (whose CRC is a valid 0,
     * which would otherwise be accepted as a bogus 0.0 C reading). */
    bool all_ff = true;
    bool all_00 = true;
    for (int i = 0; i < 9; i++) {
        all_ff = all_ff && (sp[i] == 0xFF);
        all_00 = all_00 && (sp[i] == 0x00);
    }
    if (all_ff || all_00) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (ow_crc8(sp, 8) != sp[8]) {
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

esp_err_t ds18b20_read_temperature(float *out_celsius)
{
    uint8_t   sp[9] = { 0 };
    esp_err_t err   = ESP_FAIL;

    for (int attempt = 1; attempt <= READ_ATTEMPTS; attempt++) {
        err = read_scratchpad(sp);
        if (err == ESP_OK) {
            break;
        }
        ESP_LOGW(TAG, "Read attempt %d/%d failed (%s). Scratchpad: "
                      "%02X %02X %02X %02X %02X %02X %02X %02X %02X",
                 attempt, READ_ATTEMPTS, esp_err_to_name(err),
                 sp[0], sp[1], sp[2], sp[3], sp[4], sp[5], sp[6], sp[7], sp[8]);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (err != ESP_OK) {
        return err;
    }

    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
    *out_celsius = raw / 16.0f;
    return ESP_OK;
}