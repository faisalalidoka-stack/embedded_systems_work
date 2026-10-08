#include "onewire.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Standard-speed 1-Wire slot timings in microseconds (same values as the Arduino
 * OneWire library, which the Wokwi DS18B20 model is known to work with). */
#define T_RESET_LOW      480
#define T_RESET_SAMPLE    70
#define T_RESET_END      410

#define T_WRITE1_LOW      10
#define T_WRITE1_HIGH     55
#define T_WRITE0_LOW      65
#define T_WRITE0_HIGH      5

#define T_READ_LOW         3
#define T_READ_SAMPLE     10   /* sample ~13 us after the falling edge (limit is 15 us) */
#define T_READ_END        53

static gpio_num_t s_pin = GPIO_NUM_NC;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

esp_err_t ow_init(gpio_num_t pin)
{
    s_pin = pin;
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << pin,
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_DISABLE,   /* external 4.7k pull-up */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io);
    if (err == ESP_OK) {
        gpio_set_level(s_pin, 1);
        esp_rom_delay_us(T_RESET_END);   /* let the pull-up settle before the first reset */
    }
    return err;
}

bool ow_reset(void)
{
    bool present;

    taskENTER_CRITICAL(&s_mux);
    gpio_set_level(s_pin, 0);
    esp_rom_delay_us(T_RESET_LOW);
    gpio_set_level(s_pin, 1);
    esp_rom_delay_us(T_RESET_SAMPLE);
    present = (gpio_get_level(s_pin) == 0);
    taskEXIT_CRITICAL(&s_mux);

    esp_rom_delay_us(T_RESET_END);
    return present;
}

static void write_bit(int bit)
{
    taskENTER_CRITICAL(&s_mux);
    gpio_set_level(s_pin, 0);
    if (bit) {
        esp_rom_delay_us(T_WRITE1_LOW);
        gpio_set_level(s_pin, 1);
        esp_rom_delay_us(T_WRITE1_HIGH);
    } else {
        esp_rom_delay_us(T_WRITE0_LOW);
        gpio_set_level(s_pin, 1);
        esp_rom_delay_us(T_WRITE0_HIGH);
    }
    taskEXIT_CRITICAL(&s_mux);
}

static int read_bit(void)
{
    int bit;

    taskENTER_CRITICAL(&s_mux);
    gpio_set_level(s_pin, 0);
    esp_rom_delay_us(T_READ_LOW);
    gpio_set_level(s_pin, 1);
    esp_rom_delay_us(T_READ_SAMPLE);
    bit = gpio_get_level(s_pin);
    esp_rom_delay_us(T_READ_END);
    taskEXIT_CRITICAL(&s_mux);

    return bit;
}

void ow_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        write_bit(byte & 0x01);
        byte >>= 1;
    }
}

uint8_t ow_read_byte(void)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte |= (uint8_t)(read_bit() << i);
    }
    return byte;
}

uint8_t ow_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    while (len--) {
        uint8_t b = *data++;
        for (int i = 0; i < 8; i++) {
            uint8_t mix = (crc ^ b) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            b >>= 1;
        }
    }
    return crc;
}