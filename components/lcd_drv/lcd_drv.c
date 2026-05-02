#include "lcd_drv.h"
#include "lcd_pins.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "lcd_drv";

static esp_lcd_panel_io_handle_t s_io;
static esp_lcd_panel_handle_t    s_panel;

esp_err_t lcd_drv_init(void)
{
    if (s_panel) return ESP_OK;
    gpio_config_t bl_cfg = { .mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << LCD_GPIO_BL };
    ESP_ERROR_CHECK(gpio_config(&bl_cfg));
    gpio_set_level(LCD_GPIO_BL, 1);

    spi_bus_config_t bus = {
        .mosi_io_num = LCD_GPIO_MOSI, .miso_io_num = -1, .sclk_io_num = LCD_GPIO_SCLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * 2 + 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = LCD_GPIO_DC, .cs_gpio_num = LCD_GPIO_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ, .lcd_cmd_bits = 8, .lcd_param_bits = 8,
        .spi_mode = 0, .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_cfg, &s_io));

    esp_lcd_panel_dev_config_t dev = {
        .reset_gpio_num = LCD_GPIO_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_io, &dev, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_panel, 40, 53));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    ESP_LOGI(TAG, "ST7789 ready %dx%d @ %d Hz", LCD_H_RES, LCD_V_RES, LCD_PIXEL_CLOCK_HZ);
    return ESP_OK;
}

esp_err_t lcd_drv_blit_full(const uint16_t *fb)
{
    return esp_lcd_panel_draw_bitmap(s_panel, 0, 0, LCD_H_RES, LCD_V_RES, fb);
}
