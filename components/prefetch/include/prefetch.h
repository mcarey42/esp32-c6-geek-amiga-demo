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
 * Resolution strategies for a future phase (none implemented yet):
 *   A) Tear down the LCD bus (spi_bus_free) before mounting SD, accept
 *      that LCD blits stop working from that point on (or re-init the
 *      bus back to LCD pins per scene transition — slow).
 *   B) Restructure scenes so SD is consumed at boot only (load all
 *      assets to PSRAM/internal RAM), then unmount SD and re-init the
 *      bus for LCD-only operation.
 *   C) Treat scenes as either procedural (LCD only) or asset-loading
 *      (SD only, LCD paused), with explicit bus reconfiguration at
 *      the boundary.
 *
 * TODO(phase-2): pick a strategy and implement bus re-init in
 * prefetch_mount_sd / lcd_drv_resume. Until then, calling
 * prefetch_mount_sd() AFTER lcd_drv_init() will (best case) fail to
 * mount because MISO is not configured on the active bus, or (worst
 * case) succeed and silently break LCD operation.
 */
#define SD_PIN_CMD   18   /* MOSI — only active if bus re-init'd for SD */
#define SD_PIN_CLK   19   /* SCK  — only active if bus re-init'd for SD */
#define SD_PIN_D0    20   /* MISO — only active if bus re-init'd for SD */
#define SD_PIN_CS    23   /* CS                                          */

/* Precondition: SPI bus must already be initialized by lcd_drv_init().
 * Calling this before lcd_drv_init() will fail mount with
 * ESP_ERR_INVALID_STATE.
 *
 * Known limitation (see header comment above): even after lcd_drv_init,
 * mount will likely fail because the LCD-side bus init configures
 * MISO=-1 (LCD never reads), but SD requires MISO. This needs a
 * hardware-side investigation / bus-reconfiguration strategy before
 * SD will actually work. */
int  prefetch_mount_sd(void);   /* call once at boot */

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
