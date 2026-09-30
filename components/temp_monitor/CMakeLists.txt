idf_component_register(SRCS "temp_monitor.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ds18b20 freertos esp_timer log)