idf_component_register(
    SRCS "director.c"
    INCLUDE_DIRS "include"
    REQUIRES fb esp_timer freertos esp_common log)
