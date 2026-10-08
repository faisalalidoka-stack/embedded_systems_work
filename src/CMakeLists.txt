idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES temp_monitor lcd_i2c keypad ui_controller
                                esp_driver_gpio freertos log)