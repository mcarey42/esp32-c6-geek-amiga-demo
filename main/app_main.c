#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_drv.h"
#include "fb.h"
#include "director.h"
#include "scene.h"
#include "scenes.h"
#include "prefetch.h"

extern const timeline_entry_t TIMELINE[];
extern const int               TIMELINE_LEN;

static const char *TAG = "esp32demo";

#define SIL_W      60
#define SIL_H      100
#define SIL_FRAMES 60
#define SIL_FRAME_BYTES ((SIL_W * SIL_H + 7) / 8)

void app_main(void)
{
    ESP_LOGI(TAG, "boot -- Phase 1");

    /* SD-side first: mount, pre-load all silhouette frames, unmount.
     * Then SPI2 is freed and lcd_drv_init can claim it for the LCD pins. */
    if (prefetch_mount_sd() == 0) {
        const void *frames = prefetch_load_all(
            "/sdcard/silhouettes/frame_%03u.bin", SIL_FRAMES, SIL_FRAME_BYTES);
        if (frames) {
            scene_08_silhouette_set_frames(frames);
            ESP_LOGI(TAG, "silhouette frames pre-loaded");
        } else {
            ESP_LOGW(TAG, "silhouette pre-load failed; scene 08 will show placeholder");
        }
        prefetch_unmount_sd();
    } else {
        ESP_LOGW(TAG, "no SD card; scene 08 will show placeholder");
    }

    ESP_ERROR_CHECK(lcd_drv_init());
    if (fb_init() != 0) abort();
    if (director_start(TIMELINE, TIMELINE_LEN) != 0) abort();

    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
