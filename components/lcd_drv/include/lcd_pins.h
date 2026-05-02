#pragma once

#define LCD_H_RES            240
#define LCD_V_RES            135

#define LCD_GPIO_MOSI        2
#define LCD_GPIO_SCLK        1
#define LCD_GPIO_CS          5
#define LCD_GPIO_DC          3
#define LCD_GPIO_RST         4
#define LCD_GPIO_BL          6

#define LCD_SPI_HOST         SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ   (40 * 1000 * 1000)
