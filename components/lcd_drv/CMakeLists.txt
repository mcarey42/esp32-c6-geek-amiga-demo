idf_component_register(
    SRCS "lcd_drv.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_common log)
