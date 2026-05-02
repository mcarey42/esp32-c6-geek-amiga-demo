#pragma once
#include <stdint.h>
#include <stddef.h>

#define FB_W 240
#define FB_H 135

typedef struct fb_s {
    uint16_t *pixels;   /* FB_W * FB_H RGB565 */
    int       w;        /* always FB_W */
    int       h;        /* always FB_H */
} fb_t;

/* Initialize both ping-pong buffers. Idempotent — safe to call once. */
int fb_init(void);

/* Return the back buffer the renderer should draw into next. Owned by
 * caller until fb_present() is called. */
fb_t *fb_acquire_back(void);

/* Submit the back buffer for DMA blit and rotate. After this returns,
 * the renderer must call fb_acquire_back() again to get the next buffer. */
void fb_present(void);

/* Pure helpers — also used by host tests. */
static inline uint16_t fb_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline size_t fb_index(int x, int y) {
    return (size_t)y * FB_W + (size_t)x;
}
