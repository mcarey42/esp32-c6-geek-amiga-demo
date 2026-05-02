#pragma once
#include <stdint.h>
#include <stddef.h>

/* SD pin assignments for the Waveshare ESP32-C6-GEEK board.
 *
 * Confirmed from the official Waveshare ESP-IDF SD card demo:
 *   github.com/waveshareteam/ESP32-C6-GEEK
 *     example/ESP32-C6-GEEK-Demo/ESP-IDF-5.5.1/01_SD_Card/components/sd_card_bsp/sd_card_bsp.c
 *
 * The TF (microSD) slot is wired in 1-bit SPI mode and (per the board
 * design) shares SPI2_HOST with the LCD. Bus arbitration between LCD
 * and SD is handled by separate sdspi_device / esp_lcd_panel_io devices
 * with distinct CS pins.
 */
#define SD_PIN_CMD   18   /* MOSI */
#define SD_PIN_CLK   19   /* SCK  */
#define SD_PIN_D0    20   /* MISO */
#define SD_PIN_CS    23   /* CS   */

int  prefetch_mount_sd(void);   /* call once at boot */

/* A tiny ring of frame buffers populated from a path template like
 * "/sdcard/silhouettes/frame_%03u.bin". Used by Scene 8. */
typedef struct frame_ring_s frame_ring_t;

frame_ring_t *frame_ring_open(const char *path_template,
                              int n_frames_total,
                              int frame_size_bytes,
                              int ring_capacity);
void          frame_ring_close(frame_ring_t *r);

/* Returns a pointer to the requested frame's bytes (frame_size_bytes long).
 * If not yet resident, blocks until the prefetch task has loaded it. NULL
 * if frame_index is out of range or read failed. */
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
