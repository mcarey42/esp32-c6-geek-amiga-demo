#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t lcd_drv_init(void);

/* Blit a full LCD_H_RES x LCD_V_RES RGB565 buffer. Blocking until the
 * underlying esp_lcd transaction has been queued; SPI DMA continues async.
 * Returns when the buffer is safe to overwrite. */
esp_err_t lcd_drv_blit_full(const uint16_t *fb_rgb565);
