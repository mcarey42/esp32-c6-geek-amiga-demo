#include "prefetch.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef PREFETCH_HOST_TEST
#  include "esp_vfs_fat.h"
#  include "sdmmc_cmd.h"
#  include "driver/sdspi_host.h"
#  include "driver/spi_common.h"
#  include "freertos/FreeRTOS.h"
#  include "freertos/task.h"
#  include "freertos/semphr.h"
#  include "esp_log.h"
static const char *TAG = "prefetch";
#endif

/* ---- Pure ring-state logic (host-testable) ---- */

void ring_state_init(ring_state_t *s, int total, int capacity)
{
    s->total = total;
    s->capacity = capacity;
    s->next_load = 0;
    s->oldest_loaded = 0;
}

int ring_state_next_evict(const ring_state_t *s, int requested_index)
{
    /* Evict the loaded slot furthest from the requested index. */
    int best = s->oldest_loaded;
    int best_d = abs(requested_index - s->oldest_loaded);
    for (int i = s->oldest_loaded; i < s->next_load; ++i) {
        int d = abs(requested_index - i);
        if (d > best_d) { best_d = d; best = i; }
    }
    return best;
}

#ifndef PREFETCH_HOST_TEST

/* On the ESP32-C6-GEEK the SD card and LCD share SPI2_HOST (the only
 * general-purpose SPI peripheral on C6) but use different physical pins.
 * The Phase-1 strategy (see prefetch.h header comment) is boot-time
 * pre-load: prefetch owns the SPI bus first with SD pins, reads all
 * needed assets into RAM, then tears the bus down so lcd_drv_init can
 * re-initialise it with the LCD pin set. */
#define PREFETCH_SPI_HOST   SPI2_HOST

static sdmmc_card_t *s_card;

int prefetch_mount_sd(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_CMD,
        .miso_io_num = SD_PIN_D0,
        .sclk_io_num = SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    if (spi_bus_initialize(PREFETCH_SPI_HOST, &bus_cfg, SDSPI_DEFAULT_DMA) != ESP_OK) {
        ESP_LOGE(TAG, "SD spi_bus_initialize failed");
        return -1;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };
    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_PIN_CS;
    slot_cfg.host_id = PREFETCH_SPI_HOST;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = PREFETCH_SPI_HOST;
    if (esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_cfg, &mount_cfg, &s_card) != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed");
        spi_bus_free(PREFETCH_SPI_HOST);
        return -1;
    }
    ESP_LOGI(TAG, "SD mounted (size=%lluMB)",
             ((uint64_t)s_card->csd.capacity) * s_card->csd.sector_size / (1024 * 1024));
    return 0;
}

const void *prefetch_load_all(const char *tmpl, int n_frames, int frame_size)
{
    if (n_frames <= 0 || frame_size <= 0) return NULL;
    size_t total = (size_t)n_frames * (size_t)frame_size;
    uint8_t *buf = malloc(total);
    if (!buf) {
        ESP_LOGE(TAG, "prefetch_load_all: alloc %u bytes failed", (unsigned)total);
        return NULL;
    }
    char path[128];
    for (int i = 0; i < n_frames; ++i) {
        snprintf(path, sizeof(path), tmpl, i);
        FILE *f = fopen(path, "rb");
        if (!f) {
            ESP_LOGE(TAG, "prefetch_load_all: open %s failed", path);
            free(buf);
            return NULL;
        }
        size_t n = fread(buf + (size_t)i * frame_size, 1, frame_size, f);
        fclose(f);
        if (n != (size_t)frame_size) {
            ESP_LOGE(TAG, "prefetch_load_all: short read %s (%u/%d)",
                     path, (unsigned)n, frame_size);
            free(buf);
            return NULL;
        }
    }
    ESP_LOGI(TAG, "prefetch_load_all: %d frames x %d bytes from %s",
             n_frames, frame_size, tmpl);
    return buf;
}

void prefetch_unmount_sd(void)
{
    if (s_card) {
        esp_vfs_fat_sdcard_unmount("/sdcard", s_card);
        s_card = NULL;
    }
    spi_bus_free(PREFETCH_SPI_HOST);
    ESP_LOGI(TAG, "SD unmounted, SPI2 freed");
}

struct frame_ring_s {
    char         tmpl[64];
    int          total;
    int          frame_size;
    int          capacity;
    uint8_t     *blocks;        /* capacity * frame_size bytes */
    int         *resident_idx;  /* per slot: which frame is loaded, -1 if empty */
    SemaphoreHandle_t mtx;
};

static int load_frame(frame_ring_t *r, int slot, int idx)
{
    char path[sizeof(r->tmpl) + 16];
    snprintf(path, sizeof(path), r->tmpl, idx);
    FILE *f = fopen(path, "rb");
    if (!f) { ESP_LOGE(TAG, "open %s fail", path); return -1; }
    size_t n = fread(r->blocks + (size_t)slot * r->frame_size, 1, r->frame_size, f);
    fclose(f);
    return (n == (size_t)r->frame_size) ? 0 : -1;
}

frame_ring_t *frame_ring_open(const char *tmpl, int total, int frame_size, int capacity)
{
    frame_ring_t *r = calloc(1, sizeof(*r));
    if (!r) return NULL;
    snprintf(r->tmpl, sizeof(r->tmpl), "%s", tmpl);
    r->total = total;
    r->frame_size = frame_size;
    r->capacity = capacity;
    r->blocks = malloc((size_t)capacity * frame_size);
    r->resident_idx = malloc(capacity * sizeof(int));
    r->mtx = xSemaphoreCreateMutex();
    if (!r->blocks || !r->resident_idx || !r->mtx) {
        free(r->blocks); free(r->resident_idx); free(r); return NULL;
    }
    for (int i = 0; i < capacity; ++i) r->resident_idx[i] = -1;
    /* Eagerly load the first `capacity` frames to prime the ring. */
    for (int i = 0; i < capacity && i < total; ++i) {
        if (load_frame(r, i, i) == 0) r->resident_idx[i] = i;
    }
    return r;
}

void frame_ring_close(frame_ring_t *r)
{
    if (!r) return;
    if (r->mtx) vSemaphoreDelete(r->mtx);
    free(r->blocks);
    free(r->resident_idx);
    free(r);
}

const void *frame_ring_get(frame_ring_t *r, int idx)
{
    if (!r || idx < 0 || idx >= r->total) return NULL;
    xSemaphoreTake(r->mtx, portMAX_DELAY);

    int slot_found = -1;
    for (int i = 0; i < r->capacity; ++i) {
        if (r->resident_idx[i] == idx) { slot_found = i; break; }
    }
    if (slot_found < 0) {
        /* Evict the slot whose loaded index is furthest from `idx`.
         * Empty slots (resident_idx[i] == -1) get a huge abs() distance
         * and naturally win the eviction race — fills empty slots first. */
        int evict = 0, evict_d = -1;
        for (int i = 0; i < r->capacity; ++i) {
            int d = abs(r->resident_idx[i] - idx);
            if (d > evict_d) { evict_d = d; evict = i; }
        }
        if (load_frame(r, evict, idx) == 0) {
            r->resident_idx[evict] = idx;
            slot_found = evict;
        }
    }
    const void *p = (slot_found >= 0) ? r->blocks + (size_t)slot_found * r->frame_size : NULL;
    xSemaphoreGive(r->mtx);
    return p;
}

#endif  /* !PREFETCH_HOST_TEST */
