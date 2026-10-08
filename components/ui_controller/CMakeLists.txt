idf_component_register(SRCS "ui_controller.c"
                       INCLUDE_DIRS "include"
                       REQUIRES lcd_i2c temp_monitor esp_driver_gpio freertos)