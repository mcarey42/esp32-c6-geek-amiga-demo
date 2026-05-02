idf_component_register(
    SRCS "app_main.c" "timeline.c"
    INCLUDE_DIRS "."
    REQUIRES lcd_drv fb gfx director scenes)
