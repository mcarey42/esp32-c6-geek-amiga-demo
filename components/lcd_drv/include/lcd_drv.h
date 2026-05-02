#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t lcd_drv_init(void);

/* Submit a full LCD_H_RES x LCD_V_RES RGB565 buffer for DMA blit.
 *
 * IMPORTANT: the caller MUST NOT mutate `fb_rgb565` until the *next*
 * lcd_drv_blit_full() call returns -- that next call drains the in-flight
 * DMA via a RAMWR polling drain before queueing its own data.
 *
 * For double-buffered ping-pong callers (the framebuffer manager) this
 * is automatic: each buffer gets one full frame to settle before being
 * written to again. Single-buffer callers must follow up with another
 * blit (e.g. of a scratch frame) before reusing the buffer. */
esp_err_t lcd_drv_blit_full(const uint16_t *fb_rgb565);
