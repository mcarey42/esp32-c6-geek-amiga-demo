#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_drv.h"
#include "fb.h"
#include "gfx.h"
#include "director.h"
#include "scene.h"

static const char *TAG = "esp32demo";

static void purple_render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    /* Pulse purple intensity with t, just to prove the loop runs. */
    uint8_t v = (uint8_t)(128 + 127 * (int)((t / 20) % 2));
    gfx_clear(fb, fb_rgb565(v, 0, v));
}

static const scene_t SCENE_PURPLE = {
    .name = "purple", .est_duration_ms = 5000,
    .render = purple_render,
};

static const timeline_entry_t timeline[] = {
    { .scene = &SCENE_PURPLE, .duration_ms = 5000 },
};

void app_main(void)
{
    ESP_LOGI(TAG, "boot");
    ESP_ERROR_CHECK(lcd_drv_init());
    if (fb_init() != 0) abort();
    if (director_start(timeline, 1) != 0) abort();
    ESP_LOGI(TAG, "director running");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
