#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_drv.h"
#include "fb.h"
#include "director.h"
#include "scene.h"
#include "scenes.h"

static const char *TAG = "esp32demo";

static const timeline_entry_t timeline[] = {
    { .scene = &SCENE_02_COPPER_SCROLLER, .duration_ms = 30000 },
};

void app_main(void)
{
    ESP_LOGI(TAG, "boot");
    ESP_ERROR_CHECK(lcd_drv_init());
    if (fb_init() != 0) abort();
    if (director_start(timeline, 1) != 0) abort();
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
