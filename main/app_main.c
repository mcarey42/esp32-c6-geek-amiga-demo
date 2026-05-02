#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_drv.h"
#include "fb.h"
#include "director.h"
#include "scene.h"

extern const timeline_entry_t TIMELINE[];
extern const int               TIMELINE_LEN;

static const char *TAG = "esp32demo";

void app_main(void)
{
    ESP_LOGI(TAG, "boot -- Phase 1, 2 scenes");
    ESP_ERROR_CHECK(lcd_drv_init());
    if (fb_init() != 0) abort();
    if (director_start(TIMELINE, TIMELINE_LEN) != 0) abort();
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
