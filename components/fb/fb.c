#include "fb.h"
#include "lcd_drv.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "fb";

static fb_t  s_fb[2];
static int   s_back = 0;
static bool  s_initialized = false;

int fb_init(void)
{
    if (s_initialized) return 0;
    for (int i = 0; i < 2; ++i) {
        s_fb[i].pixels = heap_caps_malloc(FB_W * FB_H * sizeof(uint16_t),
                                          MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!s_fb[i].pixels) {
            ESP_LOGE(TAG, "fb %d alloc failed", i);
            for (int j = 0; j < i; ++j) {
                heap_caps_free(s_fb[j].pixels);
                s_fb[j].pixels = NULL;
            }
            return -1;
        }
        s_fb[i].w = FB_W;
        s_fb[i].h = FB_H;
    }
    s_initialized = true;
    return 0;
}

fb_t *fb_acquire_back(void)
{
    return &s_fb[s_back];
}

esp_err_t fb_present(void)
{
    esp_err_t r = lcd_drv_blit_full(s_fb[s_back].pixels);
    s_back ^= 1;
    return r;
}
