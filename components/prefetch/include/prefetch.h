#pragma once
#include <stdint.h>
#include <stddef.h>

/* SD pin assignments for the Waveshare ESP32-C6-GEEK board.
 *
 * Confirmed from the official Waveshare ESP-IDF SD card demo:
 *   github.com/waveshareteam/ESP32-C6-GEEK
 *     example/ESP32-C6-GEEK-Demo/ESP-IDF-5.5.1/01_SD_Card/components/sd_card_bsp/sd_card_bsp.c
 *
 * !!! HARDWARE CONSTRAINT — READ BEFORE USING THE SD CARD !!!
 *
 * The ESP32-C6 has exactly ONE general-purpose SPI controller
 * (SPI2_HOST / GPSPI2). Per IDF's `spi_periph_signal[]` for esp32c6
 * (see esp_hal_gpspi/esp32c6/spi_periph.c) and SOC_SPI_PERIPH_NUM=2
 * (one MSPI for flash, one GPSPI), there is no SPI3_HOST on C6.
 *
 * The board routes the LCD on a different physical pin set
 * (MOSI=2, SCLK=1, no MISO) than the SD slot
 * (MOSI=18, SCLK=19, MISO=20, CS=23). Both Waveshare reference demos
 * (01_SD_Card and 05_lvgl_example_v8) call spi_bus_initialize(SPI2_HOST,
 * ...) themselves with their own pin set — neither demo uses both
 * peripherals simultaneously.
 *
 * Because a single SPI peripheral can only drive one set of pins via
 * the GPIO matrix at a time, LCD blits and SD reads CANNOT both work
 * after a single bus_initialize call. The pin defines below are the
 * SD-side pins; they are NOT in effect after lcd_drv_init() runs.
 *
 * CHOSEN STRATEGY (Phase 1, Task 11): boot-time pre-load.
 *   1. app_main calls prefetch_mount_sd() FIRST (it owns spi_bus_initialize
 *      with the SD pin set).
 *   2. All needed SD assets are read into heap via prefetch_load_all().
 *   3. prefetch_unmount_sd() tears down the SD mount AND calls
 *      spi_bus_free(SPI2_HOST).
 *   4. lcd_drv_init() then runs and re-initialises SPI2 with the LCD
 *      pin set. From this point on the SD card is unreachable; all SD
 *      data must already be resident in RAM.
 *
 * The frame_ring_t API below was the original Phase-1 design (intra-scene
 * streaming) and is preserved for future hardware that has two SPI hosts
 * or a separate SDIO controller. It is NOT used by Phase 1 scenes.
 */
#define SD_PIN_CMD   18   /* MOSI — active during boot-time SD pre-load */
#define SD_PIN_CLK   19   /* SCK  — active during boot-time SD pre-load */
#define SD_PIN_D0    20   /* MISO — active during boot-time SD pre-load */
#define SD_PIN_CS    23   /* CS                                         */

/* Mounts the SD card on SPI2_HOST using the SD-side pins
 * (MOSI=18, SCLK=19, MISO=20, CS=23). This function calls
 * spi_bus_initialize() itself — it must be the FIRST consumer of SPI2
 * in boot order. After SD work is done, call prefetch_unmount_sd() to
 * tear down the bus so lcd_drv_init() can re-initialize it for the LCD
 * pins. */
int  prefetch_mount_sd(void);   /* call once at boot */

/* One-shot SD loader: opens "/sdcard/<path_template>" with sprintf %u,
 * reads `n_frames` of `frame_size_bytes` each into an internally-allocated
 * buffer, returns a pointer to it (or NULL on any failure). The buffer is
 * `n_frames * frame_size_bytes` bytes; caller does not free it (we leak
 * intentionally — the data lives for the entire demo run).
 *
 * Designed for boot-time pre-loading on hardware where SPI sharing with
 * other devices makes intra-scene SD reads impractical. Call this AFTER
 * prefetch_mount_sd() and BEFORE the LCD driver re-claims the SPI bus.
 *
 * Returns NULL on any I/O failure or alloc failure. */
const void *prefetch_load_all(const char *path_template, int n_frames,
                              int frame_size_bytes);

/* Tears down the SD mount + spi_bus_free so the LCD can re-init the bus
 * with its own pin configuration. */
void prefetch_unmount_sd(void);

/* A tiny ring of frame buffers populated from a path template like
 * "/sdcard/silhouettes/frame_%03u.bin". Used by Scene 8. */
typedef struct frame_ring_s frame_ring_t;

/* On open, eagerly loads the first min(capacity, n_frames_total) frames
 * synchronously. Keep capacity * frame_size_bytes under ~8 KB to fit
 * in one frame budget when this is called from the render task. */
frame_ring_t *frame_ring_open(const char *path_template,
                              int n_frames_total,
                              int frame_size_bytes,
                              int ring_capacity);
void          frame_ring_close(frame_ring_t *r);

/* Returns a pointer to the requested frame's bytes (frame_size_bytes long).
 * If not yet resident, blocks the caller while the frame is read
 * synchronously from SD (typically 5-50ms). Task 11 may add a
 * background prefetch task; the API will not change.
 * NULL if frame_index is out of range or read failed. */
const void   *frame_ring_get(frame_ring_t *r, int frame_index);

/* Pure-pool logic exposed for host testing. Not used at runtime. */
typedef struct {
    int total;          /* total number of frames in the source */
    int capacity;       /* ring slots */
    int next_load;      /* next frame index to load */
    int oldest_loaded;  /* lowest currently-resident index */
} ring_state_t;

void ring_state_init(ring_state_t *s, int total, int capacity);
int  ring_state_next_evict(const ring_state_t *s, int requested_index);
