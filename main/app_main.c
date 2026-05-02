#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_drv.h"
#include "lcd_pins.h"

static const char *TAG = "esp32demo";

static uint16_t s_test_fb[LCD_H_RES * LCD_V_RES];

void app_main(void)
{
    ESP_LOGI(TAG, "boot");
    ESP_ERROR_CHECK(lcd_drv_init());

    /* Solid red — RGB565 0xF800. */
    for (size_t i = 0; i < LCD_H_RES * LCD_V_RES; ++i) s_test_fb[i] = 0xF800;
    ESP_ERROR_CHECK(lcd_drv_blit_full(s_test_fb));
    ESP_LOGI(TAG, "painted red");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
