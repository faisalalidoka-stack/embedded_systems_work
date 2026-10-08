#include "lcd_i2c.h"

#include <string.h>
#include "driver/i2c_master.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* PCF8574 pin mapping: P0=RS P1=RW P2=EN P3=BL P4..P7=D4..D7 */
#define PIN_RS  0x01
#define PIN_EN  0x04
#define PIN_BL  0x08

#define LCD_COLS 16

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;

static void expander_write(uint8_t v)
{
    i2c_master_transmit(s_dev, &v, 1, 50);
}

static void write_nibble(uint8_t nibble, uint8_t rs)
{
    uint8_t d = (uint8_t)((nibble << 4) | PIN_BL | rs);
    expander_write(d | PIN_EN);
    esp_rom_delay_us(2);
    expander_write(d & (uint8_t)~PIN_EN);
    esp_rom_delay_us(50);
}

static void send(uint8_t byte, uint8_t rs)
{
    write_nibble(byte >> 4, rs);
    write_nibble(byte & 0x0F, rs);
}

esp_err_t lcd_init(int sda_gpio, int scl_gpio, uint8_t i2c_addr)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = I2C_NUM_0,
        .sda_io_num        = sda_gpio,
        .scl_io_num        = scl_gpio,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (err != ESP_OK) {
        return err;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = i2c_addr,
        .scl_speed_hz    = 100000,
    };
    err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(50));          /* power-up wait */

    /* HD44780 4-bit init sequence */
    write_nibble(0x03, 0);
    esp_rom_delay_us(5000);
    write_nibble(0x03, 0);
    esp_rom_delay_us(200);
    write_nibble(0x03, 0);
    esp_rom_delay_us(200);
    write_nibble(0x02, 0);
    esp_rom_delay_us(200);

    send(0x28, 0);   /* 4-bit, 2 lines, 5x8 font */
    send(0x0C, 0);   /* display on, cursor off */
    send(0x06, 0);   /* entry mode: increment */
    lcd_clear();

    return ESP_OK;
}

void lcd_clear(void)
{
    send(0x01, 0);
    esp_rom_delay_us(2000);
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    uint8_t addr = (row ? 0x40 : 0x00) + col;
    send(0x80 | addr, 0);
}

void lcd_print(const char *text)
{
    while (*text) {
        send((uint8_t)*text++, PIN_RS);
    }
}

void lcd_print_line(uint8_t row, const char *text)
{
    char buf[LCD_COLS + 1];
    size_t n = strnlen(text, LCD_COLS);

    memcpy(buf, text, n);
    memset(buf + n, ' ', LCD_COLS - n);
    buf[LCD_COLS] = '\0';

    lcd_set_cursor(0, row);
    lcd_print(buf);
}