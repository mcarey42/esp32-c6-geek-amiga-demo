#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_drv.h"
#include "fb.h"

static const char *TAG = "esp32demo";

void app_main(void)
{
    ESP_LOGI(TAG, "boot");
    ESP_ERROR_CHECK(lcd_drv_init());
    if (fb_init() != 0) abort();

    fb_t *fb = fb_acquire_back();
    for (int i = 0; i < FB_W * FB_H; ++i) fb->pixels[i] = fb_rgb565(0, 0, 255);
    fb_present();

    ESP_LOGI(TAG, "painted blue via fb");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
