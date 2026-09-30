#include "ds18b20.h"
#include "onewire.h"

#define CMD_SKIP_ROM         0xCC
#define CMD_CONVERT_T        0x44
#define CMD_READ_SCRATCHPAD  0xBE

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

esp_err_t ds18b20_read_temperature(float *out_celsius)
{
    uint8_t sp[9];

    if (!ow_reset()) {
        return ESP_ERR_NOT_FOUND;
    }
    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_READ_SCRATCHPAD);

    for (int i = 0; i < 9; i++) {
        sp[i] = ow_read_byte();
    }

    if (ow_crc8(sp, 8) != sp[8]) {
        return ESP_ERR_INVALID_CRC;
    }

    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
    *out_celsius = raw / 16.0f;
    return ESP_OK;
}