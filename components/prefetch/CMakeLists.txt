idf_component_register(
    SRCS "prefetch.c"
    INCLUDE_DIRS "include"
    REQUIRES driver fatfs sdmmc esp_common log)
