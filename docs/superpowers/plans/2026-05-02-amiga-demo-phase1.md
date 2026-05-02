# esp32demo Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the kernel + scene-overlay framework end-to-end and ship a 3-scene mini-demo (copper bars + sine scroller, wireframe cube, SD-streamed silhouette) that loops on real ESP32-C6-Geek hardware.

**Architecture:** A `director` task drives a fixed-FPS timeline and lifecycle-manages `scene_t` modules. A `fb` ping-pong framebuffer manager owns two RGB565 buffers; one is drawn into while the other DMA-blits over SPI. A `prefetch` task lazy-loads SD assets in the background. Pure-C math, primitives, and director logic are TDD'd on the host with Unity (and a tiny PPM-snapshot helper for visual sanity); rendering is verified on hardware by eye.

**Tech Stack:** ESP-IDF v5.1+, FreeRTOS, esp_lcd (ST7789), Unity test framework (host build), CMake.

**Reference spec:** `docs/superpowers/specs/2026-05-02-amiga-demo-design.md`

---

## File Structure (created across the plan)

```
esp32demo/
├── CMakeLists.txt                          # Task 1
├── sdkconfig.defaults                      # Task 1
├── partitions.csv                          # Task 1
├── main/
│   ├── CMakeLists.txt                      # Task 1
│   ├── app_main.c                          # Task 6 (skeleton), Task 9, 12
│   └── timeline.c                          # Task 9, 12
├── components/
│   ├── lcd_drv/                            # Task 2
│   │   ├── include/lcd_pins.h
│   │   ├── include/lcd_drv.h
│   │   ├── lcd_drv.c
│   │   └── CMakeLists.txt
│   ├── fb/                                 # Task 3
│   │   ├── include/fb.h
│   │   ├── fb.c
│   │   └── CMakeLists.txt
│   ├── gfx/                                # Task 4
│   │   ├── include/gfx.h
│   │   ├── gfx.c
│   │   ├── font_5x7.c
│   │   └── CMakeLists.txt
│   ├── gfx3d/                              # Task 5
│   │   ├── include/gfx3d.h
│   │   ├── gfx3d.c
│   │   └── CMakeLists.txt
│   ├── director/                           # Task 6
│   │   ├── include/director.h
│   │   ├── include/scene.h
│   │   ├── director.c
│   │   ├── transitions.c                   # Task 9
│   │   └── CMakeLists.txt
│   ├── prefetch/                           # Task 10
│   │   ├── include/prefetch.h
│   │   ├── prefetch.c
│   │   └── CMakeLists.txt
│   └── scenes/                             # Tasks 7, 8, 11
│       ├── include/scenes.h
│       ├── scene_02_copper_scroller.c
│       ├── scene_04_cube.c
│       ├── scene_08_silhouette.c
│       └── CMakeLists.txt
├── test_host/                              # Task 1
│   ├── CMakeLists.txt
│   ├── unity/                              # vendored from IDF
│   ├── test_fb.c                           # Task 3
│   ├── test_gfx.c                          # Task 4
│   ├── test_gfx3d.c                        # Task 5
│   ├── test_director.c                     # Task 6
│   ├── test_scene_02.c                     # Task 7
│   ├── test_scene_04.c                     # Task 8
│   ├── test_prefetch.c                     # Task 10
│   ├── test_scene_08.c                     # Task 11
│   └── ppm.c                               # Task 4 (PPM snapshot helper)
└── sdcard/                                 # Task 11 (files copied to physical SD)
    └── silhouettes/
        └── frame_*.bin
```

---

## Task 1: Project skeleton + host test scaffold

**Files:**
- Create: `CMakeLists.txt`
- Create: `sdkconfig.defaults`
- Create: `partitions.csv`
- Create: `main/CMakeLists.txt`
- Create: `main/app_main.c`
- Create: `test_host/CMakeLists.txt`
- Create: `test_host/unity/unity.c`, `unity/unity.h`, `unity/unity_internals.h` (vendored)
- Create: `test_host/test_smoke.c`
- Create: `test_host/run.sh`

- [ ] **Step 1: Create the IDF project root files**

`CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32demo)
```

`sdkconfig.defaults`:
```
CONFIG_IDF_TARGET="esp32c6"
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_FREERTOS_HZ=1000
CONFIG_COMPILER_OPTIMIZATION_PERF=y
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
```

`partitions.csv`:
```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x300000,
```

`main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS ".")
```

`main/app_main.c`:
```c
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "esp32demo";

void app_main(void)
{
    ESP_LOGI(TAG, "esp32demo boot — Phase 1 skeleton");
}
```

- [ ] **Step 2: Verify the IDF build works on the C6 target**

Run: `idf.py set-target esp32c6 && idf.py build`
Expected: build succeeds, `build/esp32demo.bin` produced.

- [ ] **Step 3: Set up the host test scaffold**

Create `test_host/CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.16)
project(esp32demo_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -O0 -g")

include_directories(
    unity
    ../components/fb/include
    ../components/gfx/include
    ../components/gfx3d/include
    ../components/director/include
    ../components/prefetch/include
    ../components/scenes/include
)

add_library(unity STATIC unity/unity.c)

# Helper for snapshot tests
add_library(ppm STATIC ppm.c)

# Each test binary is added by later tasks. The smoke test exists from day one.
add_executable(test_smoke test_smoke.c)
target_link_libraries(test_smoke unity)

enable_testing()
add_test(NAME smoke COMMAND test_smoke)
```

Vendor Unity from `$IDF_PATH/components/unity/unity/src/`:
```bash
cp $IDF_PATH/components/unity/unity/src/unity.c        test_host/unity/
cp $IDF_PATH/components/unity/unity/src/unity.h        test_host/unity/
cp $IDF_PATH/components/unity/unity/src/unity_internals.h test_host/unity/
```

`test_host/test_smoke.c`:
```c
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_smoke_passes(void) {
    TEST_ASSERT_EQUAL_INT(2, 1 + 1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_smoke_passes);
    return UNITY_END();
}
```

`test_host/ppm.c` (stub, real impl in Task 4):
```c
/* PPM snapshot helper — implemented in Task 4. */
```

`test_host/run.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

```bash
chmod +x test_host/run.sh
```

- [ ] **Step 4: Run the host smoke test**

Run: `./test_host/run.sh`
Expected: smoke test passes, output ends with `1 Tests 0 Failures 0 Ignored OK`.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt sdkconfig.defaults partitions.csv main/ test_host/
git commit -m "Task 1: IDF project skeleton + Unity host test scaffold"
```

---

## Task 2: LCD driver (`lcd_drv` component)

A slim ST7789 driver: init the panel, blit one full-width strip via DMA. No LVGL. Adapted from `../crypto-tests/components/ui_lcd/` but stripped to its essence.

**Files:**
- Create: `components/lcd_drv/include/lcd_pins.h`
- Create: `components/lcd_drv/include/lcd_drv.h`
- Create: `components/lcd_drv/lcd_drv.c`
- Create: `components/lcd_drv/CMakeLists.txt`
- Modify: `main/app_main.c`
- Modify: `main/CMakeLists.txt`

This task has no host tests — its correctness is "you see something on the screen."

- [ ] **Step 1: Pin defs (lifted from crypto-tests)**

`components/lcd_drv/include/lcd_pins.h`:
```c
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
```

- [ ] **Step 2: Public API**

`components/lcd_drv/include/lcd_drv.h`:
```c
#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t lcd_drv_init(void);

/* Blit a full LCD_H_RES x LCD_V_RES RGB565 buffer. Blocking until the
 * underlying esp_lcd transaction has been queued; SPI DMA continues async.
 * Returns when the buffer is safe to overwrite. */
esp_err_t lcd_drv_blit_full(const uint16_t *fb_rgb565);
```

- [ ] **Step 3: Implementation**

`components/lcd_drv/lcd_drv.c`:
```c
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
```

`components/lcd_drv/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "lcd_drv.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_common log)
```

- [ ] **Step 4: Smoke test on hardware — paint the screen red**

Modify `main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES lcd_drv)
```

Replace `main/app_main.c`:
```c
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_drv.h"
#include "lcd_pins.h"

static const char *TAG = "esp32demo";

static uint16_t s_test_fb[LCD_H_RES * LCD_V_RES];

void app_main(void)
{
    ESP_LOGI(TAG, "boot");
    ESP_ERROR_CHECK(lcd_drv_init());

    /* Solid red — RGB565 0xF800. */
    for (size_t i = 0; i < LCD_H_RES * LCD_V_RES; ++i) s_test_fb[i] = 0xF800;
    ESP_ERROR_CHECK(lcd_drv_blit_full(s_test_fb));
    ESP_LOGI(TAG, "painted red");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
```

- [ ] **Step 5: Build, flash, and verify by eye**

Run: `idf.py build flash monitor`
Expected: device boots, log shows "ST7789 ready 240x135", screen is solid red. Confirm by eye, then `Ctrl+]` to exit monitor.

- [ ] **Step 6: Commit**

```bash
git add components/lcd_drv main/
git commit -m "Task 2: ST7789 driver + red-screen smoke test on hw"
```

---

## Task 3: Framebuffer manager (`fb` component)

Owns two ping-pong RGB565 framebuffers in SRAM. Exposes `fb_acquire_back()` to the renderer and `fb_present()` to swap and DMA-blit.

**Files:**
- Create: `components/fb/include/fb.h`
- Create: `components/fb/fb.c`
- Create: `components/fb/CMakeLists.txt`
- Create: `test_host/test_fb.c`
- Modify: `test_host/CMakeLists.txt`
- Modify: `main/app_main.c`

- [ ] **Step 1: Public API**

`components/fb/include/fb.h`:
```c
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
```

- [ ] **Step 2: Write failing host tests for the pure helpers**

`test_host/test_fb.c`:
```c
#include "unity.h"
#include "fb.h"

void setUp(void) {}
void tearDown(void) {}

void test_rgb565_pure_red(void) {
    TEST_ASSERT_EQUAL_HEX16(0xF800, fb_rgb565(255, 0, 0));
}

void test_rgb565_pure_green(void) {
    TEST_ASSERT_EQUAL_HEX16(0x07E0, fb_rgb565(0, 255, 0));
}

void test_rgb565_pure_blue(void) {
    TEST_ASSERT_EQUAL_HEX16(0x001F, fb_rgb565(0, 0, 255));
}

void test_rgb565_white(void) {
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, fb_rgb565(255, 255, 255));
}

void test_index_origin(void) {
    TEST_ASSERT_EQUAL_size_t(0, fb_index(0, 0));
}

void test_index_row_step(void) {
    TEST_ASSERT_EQUAL_size_t(FB_W, fb_index(0, 1));
}

void test_index_arbitrary(void) {
    TEST_ASSERT_EQUAL_size_t(10 + 5 * FB_W, fb_index(10, 5));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rgb565_pure_red);
    RUN_TEST(test_rgb565_pure_green);
    RUN_TEST(test_rgb565_pure_blue);
    RUN_TEST(test_rgb565_white);
    RUN_TEST(test_index_origin);
    RUN_TEST(test_index_row_step);
    RUN_TEST(test_index_arbitrary);
    return UNITY_END();
}
```

Add to `test_host/CMakeLists.txt` after the `test_smoke` line:
```cmake
add_executable(test_fb test_fb.c)
target_link_libraries(test_fb unity)
add_test(NAME fb COMMAND test_fb)
```

- [ ] **Step 3: Run host tests to confirm they fail to build**

Run: `./test_host/run.sh`
Expected: build fails — `fb.h` not found. (The header doesn't exist yet because it's in `components/fb/include/` and the include path is correct, but we wrote the include path before creating the header.) **If** the include path resolves and the helpers are inline, this will actually pass (since the helpers are header-only). That's fine — go to Step 4.

- [ ] **Step 4: Create the `fb_init` / `fb_acquire_back` / `fb_present` implementation**

`components/fb/fb.c`:
```c
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

void fb_present(void)
{
    lcd_drv_blit_full(s_fb[s_back].pixels);
    s_back ^= 1;
}
```

`components/fb/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "fb.c"
    INCLUDE_DIRS "include"
    REQUIRES lcd_drv esp_common log)
```

- [ ] **Step 5: Run host tests — expect pass**

Run: `./test_host/run.sh`
Expected: 7/7 fb tests pass.

- [ ] **Step 6: On-target smoke — solid blue via fb path**

Modify `main/CMakeLists.txt` to add `fb` to REQUIRES:
```cmake
idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES lcd_drv fb)
```

Replace the loop in `main/app_main.c`:
```c
#include "fb.h"
/* ... existing includes ... */

void app_main(void)
{
    ESP_LOGI(TAG, "boot");
    ESP_ERROR_CHECK(lcd_drv_init());
    if (fb_init() != 0) abort();

    fb_t *fb = fb_acquire_back();
    for (int i = 0; i < FB_W * FB_H; ++i) fb->pixels[i] = fb_rgb565(0, 0, 255);
    fb_present();

    ESP_LOGI(TAG, "painted blue via fb");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
```

- [ ] **Step 7: Build, flash, verify by eye**

Run: `idf.py build flash monitor`
Expected: screen is solid blue. Free heap log line shows reasonable values (>200 KB free after init).

- [ ] **Step 8: Commit**

```bash
git add components/fb test_host/test_fb.c test_host/CMakeLists.txt main/
git commit -m "Task 3: ping-pong framebuffer manager + RGB565/index host tests"
```

---

## Task 4: 2D primitives (`gfx`) + PPM snapshot helper

Pure-pixel primitives: clear, hline, vline, line (Bresenham), rect, blit_rect, draw_text_5x7. All TDD'd by writing into a small scratch fb on the host and either asserting pixel values directly or writing a PPM snapshot for visual review.

**Files:**
- Create: `components/gfx/include/gfx.h`
- Create: `components/gfx/gfx.c`
- Create: `components/gfx/font_5x7.c`
- Create: `components/gfx/CMakeLists.txt`
- Create: `test_host/test_gfx.c`
- Modify: `test_host/ppm.c` (real implementation)
- Modify: `test_host/CMakeLists.txt`

- [ ] **Step 1: Implement the PPM snapshot helper**

`test_host/ppm.c`:
```c
#include <stdio.h>
#include <stdint.h>

/* Write a P6 PPM file from an RGB565 buffer. Used by tests to snapshot
 * rendered output for human inspection. Returns 0 on success. */
int ppm_write_rgb565(const char *path, const uint16_t *fb, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; ++i) {
        uint16_t p = fb[i];
        uint8_t r = (uint8_t)(((p >> 11) & 0x1F) * 255 / 31);
        uint8_t g = (uint8_t)(((p >> 5)  & 0x3F) * 255 / 63);
        uint8_t b = (uint8_t)((p & 0x1F)  * 255 / 31);
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
    return 0;
}

/* Header-style declaration used by tests via extern. */
```

Add to top of any test that uses it:
```c
extern int ppm_write_rgb565(const char *path, const uint16_t *fb, int w, int h);
```

- [ ] **Step 2: Public API for gfx primitives**

`components/gfx/include/gfx.h`:
```c
#pragma once
#include <stdint.h>
#include "fb.h"

void gfx_clear   (fb_t *fb, uint16_t color);
void gfx_pixel   (fb_t *fb, int x, int y, uint16_t color);
void gfx_hline   (fb_t *fb, int x, int y, int len, uint16_t color);
void gfx_vline   (fb_t *fb, int x, int y, int len, uint16_t color);
void gfx_line    (fb_t *fb, int x0, int y0, int x1, int y1, uint16_t color);
void gfx_rect    (fb_t *fb, int x, int y, int w, int h, uint16_t color);
void gfx_text_5x7(fb_t *fb, int x, int y, const char *s, uint16_t color);
```

- [ ] **Step 3: Write failing tests for clear, pixel, hline, vline**

`test_host/test_gfx.c`:
```c
#include "unity.h"
#include "gfx.h"
#include <stdint.h>
#include <string.h>

extern int ppm_write_rgb565(const char *path, const uint16_t *fb, int w, int h);

static uint16_t s_pixels[FB_W * FB_H];
static fb_t s_fb = { .pixels = s_pixels, .w = FB_W, .h = FB_H };

void setUp(void)    { memset(s_pixels, 0, sizeof(s_pixels)); }
void tearDown(void) {}

void test_clear_fills_all_pixels(void) {
    gfx_clear(&s_fb, 0xABCD);
    for (int i = 0; i < FB_W * FB_H; ++i)
        TEST_ASSERT_EQUAL_HEX16(0xABCD, s_pixels[i]);
}

void test_pixel_writes_single_location(void) {
    gfx_pixel(&s_fb, 10, 20, 0xBEEF);
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, s_pixels[20 * FB_W + 10]);
    TEST_ASSERT_EQUAL_HEX16(0,      s_pixels[20 * FB_W + 11]);
    TEST_ASSERT_EQUAL_HEX16(0,      s_pixels[19 * FB_W + 10]);
}

void test_pixel_clips_oob(void) {
    gfx_pixel(&s_fb, -1,    0, 0xFFFF);
    gfx_pixel(&s_fb, FB_W,  0, 0xFFFF);
    gfx_pixel(&s_fb, 0,    -1, 0xFFFF);
    gfx_pixel(&s_fb, 0, FB_H,  0xFFFF);
    /* No assertion needed — we just must not crash or write OOB. */
}

void test_hline_writes_n_pixels(void) {
    gfx_hline(&s_fb, 5, 10, 7, 0x1234);
    for (int i = 0; i < 7; ++i)
        TEST_ASSERT_EQUAL_HEX16(0x1234, s_pixels[10 * FB_W + 5 + i]);
    TEST_ASSERT_EQUAL_HEX16(0, s_pixels[10 * FB_W + 5 + 7]);
}

void test_hline_clips_at_right_edge(void) {
    gfx_hline(&s_fb, FB_W - 3, 0, 100, 0xAAAA);
    TEST_ASSERT_EQUAL_HEX16(0xAAAA, s_pixels[FB_W - 3]);
    TEST_ASSERT_EQUAL_HEX16(0xAAAA, s_pixels[FB_W - 1]);
    /* y=1 row 0 must remain untouched */
    TEST_ASSERT_EQUAL_HEX16(0, s_pixels[FB_W]);
}

void test_vline_writes_n_pixels(void) {
    gfx_vline(&s_fb, 50, 10, 5, 0x5678);
    for (int i = 0; i < 5; ++i)
        TEST_ASSERT_EQUAL_HEX16(0x5678, s_pixels[(10 + i) * FB_W + 50]);
}

void test_line_diagonal_endpoints(void) {
    gfx_line(&s_fb, 0, 0, 9, 9, 0xFFFF);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, s_pixels[0]);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, s_pixels[9 * FB_W + 9]);
}

void test_rect_outline_only(void) {
    gfx_rect(&s_fb, 10, 10, 5, 5, 0xCAFE);
    /* Top edge */
    TEST_ASSERT_EQUAL_HEX16(0xCAFE, s_pixels[10 * FB_W + 10]);
    TEST_ASSERT_EQUAL_HEX16(0xCAFE, s_pixels[10 * FB_W + 14]);
    /* Bottom edge */
    TEST_ASSERT_EQUAL_HEX16(0xCAFE, s_pixels[14 * FB_W + 10]);
    /* Interior MUST be untouched */
    TEST_ASSERT_EQUAL_HEX16(0,      s_pixels[12 * FB_W + 12]);
}

void test_text_renders_some_pixels(void) {
    gfx_text_5x7(&s_fb, 0, 0, "A", 0xFFFF);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    /* Letter 'A' in a 5x7 cell sets at least 8 pixels. */
    TEST_ASSERT_TRUE_MESSAGE(set >= 8, "expected >=8 lit pixels for 'A'");
    /* Snapshot for human review. */
    ppm_write_rgb565("/tmp/test_text_A.ppm", s_pixels, FB_W, FB_H);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_clear_fills_all_pixels);
    RUN_TEST(test_pixel_writes_single_location);
    RUN_TEST(test_pixel_clips_oob);
    RUN_TEST(test_hline_writes_n_pixels);
    RUN_TEST(test_hline_clips_at_right_edge);
    RUN_TEST(test_vline_writes_n_pixels);
    RUN_TEST(test_line_diagonal_endpoints);
    RUN_TEST(test_rect_outline_only);
    RUN_TEST(test_text_renders_some_pixels);
    return UNITY_END();
}
```

Add to `test_host/CMakeLists.txt`:
```cmake
# Add gfx sources to the host build (need the .c, not just headers)
add_executable(test_gfx test_gfx.c ../components/gfx/gfx.c ../components/gfx/font_5x7.c)
target_link_libraries(test_gfx unity ppm)
add_test(NAME gfx COMMAND test_gfx)
```

- [ ] **Step 4: Run — expect all gfx tests to fail (linker errors, no impl)**

Run: `./test_host/run.sh`
Expected: build fails because `gfx.c` and `font_5x7.c` don't exist yet.

- [ ] **Step 5: Implement `gfx.c`**

`components/gfx/gfx.c`:
```c
#include "gfx.h"
#include <string.h>
#include <stdlib.h>

extern const uint8_t font5x7[96][5];  /* defined in font_5x7.c */

void gfx_clear(fb_t *fb, uint16_t color)
{
    for (int i = 0; i < fb->w * fb->h; ++i) fb->pixels[i] = color;
}

void gfx_pixel(fb_t *fb, int x, int y, uint16_t color)
{
    if ((unsigned)x >= (unsigned)fb->w || (unsigned)y >= (unsigned)fb->h) return;
    fb->pixels[y * fb->w + x] = color;
}

void gfx_hline(fb_t *fb, int x, int y, int len, uint16_t color)
{
    if ((unsigned)y >= (unsigned)fb->h) return;
    if (x < 0)            { len += x; x = 0; }
    if (x + len > fb->w)  { len = fb->w - x; }
    if (len <= 0) return;
    uint16_t *p = &fb->pixels[y * fb->w + x];
    for (int i = 0; i < len; ++i) p[i] = color;
}

void gfx_vline(fb_t *fb, int x, int y, int len, uint16_t color)
{
    if ((unsigned)x >= (unsigned)fb->w) return;
    if (y < 0)            { len += y; y = 0; }
    if (y + len > fb->h)  { len = fb->h - y; }
    if (len <= 0) return;
    uint16_t *p = &fb->pixels[y * fb->w + x];
    for (int i = 0; i < len; ++i) { *p = color; p += fb->w; }
}

void gfx_line(fb_t *fb, int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        gfx_pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_rect(fb_t *fb, int x, int y, int w, int h, uint16_t color)
{
    gfx_hline(fb, x,         y,         w, color);
    gfx_hline(fb, x,         y + h - 1, w, color);
    gfx_vline(fb, x,         y,         h, color);
    gfx_vline(fb, x + w - 1, y,         h, color);
}

void gfx_text_5x7(fb_t *fb, int x, int y, const char *s, uint16_t color)
{
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c < 32 || c > 127) c = '?';
        const uint8_t *glyph = font5x7[c - 32];
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; ++row) {
                if (bits & (1 << row)) gfx_pixel(fb, x + col, y + row, color);
            }
        }
        x += 6;  /* 5 px glyph + 1 px spacing */
    }
}
```

`components/gfx/font_5x7.c` (minimal — start with space, A, B, '!', '0'-'9', and a wildcard '?' so tests pass; full font filled in over time):
```c
#include <stdint.h>

/* 5 columns, each column is a 7-bit bitmap (LSB = top row).
 * Indexed by (ascii - 32). 96 entries cover ' ' through '~'. */
const uint8_t font5x7[96][5] = {
    [0]  = {0,0,0,0,0},                          /* space */
    [1]  = {0x5F,0,0,0,0},                       /* ! */
    [16] = {0x3E,0x51,0x49,0x45,0x3E},           /* 0 */
    [17] = {0,0x42,0x7F,0x40,0},                 /* 1 */
    [18] = {0x42,0x61,0x51,0x49,0x46},           /* 2 */
    [19] = {0x21,0x41,0x45,0x4B,0x31},           /* 3 */
    [20] = {0x18,0x14,0x12,0x7F,0x10},           /* 4 */
    [21] = {0x27,0x45,0x45,0x45,0x39},           /* 5 */
    [22] = {0x3C,0x4A,0x49,0x49,0x30},           /* 6 */
    [23] = {0x01,0x71,0x09,0x05,0x03},           /* 7 */
    [24] = {0x36,0x49,0x49,0x49,0x36},           /* 8 */
    [25] = {0x06,0x49,0x49,0x29,0x1E},           /* 9 */
    [31] = {0x02,0x01,0x51,0x09,0x06},           /* ? */
    [33] = {0x7E,0x11,0x11,0x11,0x7E},           /* A */
    [34] = {0x7F,0x49,0x49,0x49,0x36},           /* B */
    /* Fill in the rest of A-Z, a-z, and remaining punctuation as scenes
       need them. The wildcard '?' (index 31) renders for anything missing. */
};
```

`components/gfx/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "gfx.c" "font_5x7.c"
    INCLUDE_DIRS "include"
    REQUIRES fb)
```

- [ ] **Step 6: Run host tests, expect green**

Run: `./test_host/run.sh`
Expected: `gfx` test prints `9 Tests 0 Failures 0 Ignored OK`. Inspect `/tmp/test_text_A.ppm` (`feh` or `display` it) — should show a small white "A" in the top-left corner.

- [ ] **Step 7: Commit**

```bash
git add components/gfx test_host/test_gfx.c test_host/ppm.c test_host/CMakeLists.txt
git commit -m "Task 4: gfx 2D primitives (clear/line/rect/text) + PPM snapshot tests"
```

---

## Task 5: 3D math (`gfx3d`)

Vec3, mat4, perspective projection, edge-list draw. All TDD'd on host.

**Files:**
- Create: `components/gfx3d/include/gfx3d.h`
- Create: `components/gfx3d/gfx3d.c`
- Create: `components/gfx3d/CMakeLists.txt`
- Create: `test_host/test_gfx3d.c`
- Modify: `test_host/CMakeLists.txt`

- [ ] **Step 1: Public API**

`components/gfx3d/include/gfx3d.h`:
```c
#pragma once
#include <stdint.h>
#include "fb.h"

typedef struct { float x, y, z; } vec3_t;
typedef struct { float m[16]; }   mat4_t;          /* column-major */

mat4_t mat4_identity   (void);
mat4_t mat4_mul        (mat4_t a, mat4_t b);
mat4_t mat4_rotation_x (float radians);
mat4_t mat4_rotation_y (float radians);
mat4_t mat4_rotation_z (float radians);
mat4_t mat4_translation(float tx, float ty, float tz);
mat4_t mat4_perspective(float fov_y_radians, float aspect, float near_, float far_);

vec3_t mat4_transform_point(mat4_t m, vec3_t v);

/* Project a transformed point into framebuffer coords. Returns 0 if behind
 * the near plane (caller should not draw). x/y are written on success. */
int gfx3d_project(vec3_t v_view, mat4_t proj, int *out_x, int *out_y);

/* Render a wireframe model: vertex array + edge index pairs. */
typedef struct {
    const vec3_t  *verts;
    const uint16_t *edges;   /* pairs of indices: [a0,b0, a1,b1, ...] */
    int             n_edges;
} model_t;

void gfx3d_draw_wireframe(fb_t *fb, const model_t *m, mat4_t world,
                          mat4_t proj, uint16_t color);
```

- [ ] **Step 2: Failing tests**

`test_host/test_gfx3d.c`:
```c
#include "unity.h"
#include "gfx3d.h"
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

#define EPS 1e-5f

void test_identity_times_identity(void) {
    mat4_t i = mat4_identity();
    mat4_t r = mat4_mul(i, i);
    for (int k = 0; k < 16; ++k) {
        TEST_ASSERT_FLOAT_WITHIN(EPS, i.m[k], r.m[k]);
    }
}

void test_translation_moves_point(void) {
    mat4_t t = mat4_translation(1.0f, 2.0f, 3.0f);
    vec3_t v = mat4_transform_point(t, (vec3_t){0, 0, 0});
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, v.x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, v.y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, v.z);
}

void test_rotation_y_90_degrees_swaps_x_z(void) {
    mat4_t r = mat4_rotation_y((float)M_PI / 2.0f);
    vec3_t v = mat4_transform_point(r, (vec3_t){1, 0, 0});
    /* Rotating (1,0,0) by 90deg around Y → (0,0,-1) */
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f,  v.x);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f,  v.y);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -1.0f, v.z);
}

void test_perspective_projects_centered_point_to_screen_center(void) {
    mat4_t p = mat4_perspective((float)M_PI / 3.0f, (float)FB_W / FB_H, 0.1f, 100.0f);
    int sx, sy;
    int ok = gfx3d_project((vec3_t){0, 0, -5}, p, &sx, &sy);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_INT_WITHIN(2, FB_W / 2, sx);
    TEST_ASSERT_INT_WITHIN(2, FB_H / 2, sy);
}

void test_perspective_rejects_behind_near(void) {
    mat4_t p = mat4_perspective((float)M_PI / 3.0f, (float)FB_W / FB_H, 0.1f, 100.0f);
    int sx, sy;
    int ok = gfx3d_project((vec3_t){0, 0, 1.0f}, p, &sx, &sy);
    TEST_ASSERT_FALSE(ok);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_identity_times_identity);
    RUN_TEST(test_translation_moves_point);
    RUN_TEST(test_rotation_y_90_degrees_swaps_x_z);
    RUN_TEST(test_perspective_projects_centered_point_to_screen_center);
    RUN_TEST(test_perspective_rejects_behind_near);
    return UNITY_END();
}
```

Add to `test_host/CMakeLists.txt`:
```cmake
add_executable(test_gfx3d test_gfx3d.c ../components/gfx3d/gfx3d.c ../components/gfx/gfx.c ../components/gfx/font_5x7.c)
target_link_libraries(test_gfx3d unity m)
add_test(NAME gfx3d COMMAND test_gfx3d)
```

- [ ] **Step 3: Run, expect link failure**

Run: `./test_host/run.sh`
Expected: link fails — `gfx3d.c` doesn't exist.

- [ ] **Step 4: Implement `gfx3d.c`**

`components/gfx3d/gfx3d.c`:
```c
#include "gfx3d.h"
#include "gfx.h"
#include <math.h>
#include <string.h>

/* Column-major 4x4. Element (row r, col c) is m[c*4 + r]. */
#define M(mat, r, c) ((mat).m[(c)*4 + (r)])

mat4_t mat4_identity(void)
{
    mat4_t r = {{0}};
    for (int i = 0; i < 4; ++i) M(r, i, i) = 1.0f;
    return r;
}

mat4_t mat4_mul(mat4_t a, mat4_t b)
{
    mat4_t r;
    for (int c = 0; c < 4; ++c)
        for (int rr = 0; rr < 4; ++rr) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += M(a, rr, k) * M(b, k, c);
            M(r, rr, c) = s;
        }
    return r;
}

mat4_t mat4_rotation_x(float t)
{
    float c = cosf(t), s = sinf(t);
    mat4_t r = mat4_identity();
    M(r, 1, 1) =  c; M(r, 1, 2) = -s;
    M(r, 2, 1) =  s; M(r, 2, 2) =  c;
    return r;
}

mat4_t mat4_rotation_y(float t)
{
    float c = cosf(t), s = sinf(t);
    mat4_t r = mat4_identity();
    M(r, 0, 0) =  c; M(r, 0, 2) =  s;
    M(r, 2, 0) = -s; M(r, 2, 2) =  c;
    return r;
}

mat4_t mat4_rotation_z(float t)
{
    float c = cosf(t), s = sinf(t);
    mat4_t r = mat4_identity();
    M(r, 0, 0) =  c; M(r, 0, 1) = -s;
    M(r, 1, 0) =  s; M(r, 1, 1) =  c;
    return r;
}

mat4_t mat4_translation(float tx, float ty, float tz)
{
    mat4_t r = mat4_identity();
    M(r, 0, 3) = tx; M(r, 1, 3) = ty; M(r, 2, 3) = tz;
    return r;
}

mat4_t mat4_perspective(float fov_y, float aspect, float n, float f)
{
    float t = 1.0f / tanf(fov_y * 0.5f);
    mat4_t r = {{0}};
    M(r, 0, 0) = t / aspect;
    M(r, 1, 1) = t;
    M(r, 2, 2) = (f + n) / (n - f);
    M(r, 2, 3) = (2.0f * f * n) / (n - f);
    M(r, 3, 2) = -1.0f;
    return r;
}

vec3_t mat4_transform_point(mat4_t m, vec3_t v)
{
    float x = M(m,0,0)*v.x + M(m,0,1)*v.y + M(m,0,2)*v.z + M(m,0,3);
    float y = M(m,1,0)*v.x + M(m,1,1)*v.y + M(m,1,2)*v.z + M(m,1,3);
    float z = M(m,2,0)*v.x + M(m,2,1)*v.y + M(m,2,2)*v.z + M(m,2,3);
    return (vec3_t){x, y, z};
}

int gfx3d_project(vec3_t v_view, mat4_t proj, int *out_x, int *out_y)
{
    /* v_view is camera-space (looking down -Z). Reject if z >= -near. */
    if (v_view.z >= -0.05f) return 0;

    float x = M(proj,0,0)*v_view.x + M(proj,0,2)*v_view.z;
    float y = M(proj,1,1)*v_view.y + M(proj,1,2)*v_view.z;
    float w = M(proj,3,2)*v_view.z;
    if (w == 0) return 0;
    float ndc_x = x / w;
    float ndc_y = y / w;
    *out_x = (int)((ndc_x * 0.5f + 0.5f) * FB_W);
    *out_y = (int)((1.0f - (ndc_y * 0.5f + 0.5f)) * FB_H);
    return 1;
}

void gfx3d_draw_wireframe(fb_t *fb, const model_t *m, mat4_t world,
                          mat4_t proj, uint16_t color)
{
    for (int e = 0; e < m->n_edges; ++e) {
        uint16_t a = m->edges[e * 2 + 0];
        uint16_t b = m->edges[e * 2 + 1];
        vec3_t va = mat4_transform_point(world, m->verts[a]);
        vec3_t vb = mat4_transform_point(world, m->verts[b]);
        int xa, ya, xb, yb;
        if (gfx3d_project(va, proj, &xa, &ya) &&
            gfx3d_project(vb, proj, &xb, &yb)) {
            gfx_line(fb, xa, ya, xb, yb, color);
        }
    }
}
```

`components/gfx3d/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "gfx3d.c"
    INCLUDE_DIRS "include"
    REQUIRES fb gfx)
```

- [ ] **Step 5: Run, expect green**

Run: `./test_host/run.sh`
Expected: `gfx3d` shows `5 Tests 0 Failures 0 Ignored OK`.

- [ ] **Step 6: Commit**

```bash
git add components/gfx3d test_host/test_gfx3d.c test_host/CMakeLists.txt
git commit -m "Task 5: 3D math (vec3/mat4/perspective/wireframe) + host tests"
```

---

## Task 6: Director (`director`) + scene API

Owns the timeline cursor, lifecycle (init/render/teardown), per-frame pacing, and the render task. Single-scene path first — multi-scene + transitions arrive in Task 9.

**Files:**
- Create: `components/director/include/scene.h`
- Create: `components/director/include/director.h`
- Create: `components/director/director.c`
- Create: `components/director/CMakeLists.txt`
- Create: `test_host/test_director.c`
- Modify: `test_host/CMakeLists.txt`
- Modify: `main/app_main.c`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Scene API header**

`components/director/include/scene.h`:
```c
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "fb.h"

typedef struct asset_manifest_s asset_manifest_t;  /* defined later by prefetch */

typedef struct scene_s {
    const char *name;
    uint32_t    est_duration_ms;
    const asset_manifest_t *assets;          /* NULL if none */

    int   (*init)    (void *ctx);
    void  (*render)  (void *ctx, fb_t *fb, uint32_t scene_t_ms);
    void  (*teardown)(void *ctx);
    size_t ctx_size;
} scene_t;

typedef struct timeline_entry_s {
    const scene_t *scene;       /* NULL means "transition" — Task 9 */
    int            transition;  /* unused until Task 9 */
    uint32_t       duration_ms;
} timeline_entry_t;
```

- [ ] **Step 2: Director header**

`components/director/include/director.h`:
```c
#pragma once
#include "scene.h"

/* Pure-logic timeline cursor — extracted from the FreeRTOS task so it
 * can be tested on the host. */
typedef struct {
    const timeline_entry_t *entries;
    int                     n_entries;
    int                     idx;
    uint32_t                t_in_scene_ms;
} cursor_t;

void cursor_init   (cursor_t *c, const timeline_entry_t *entries, int n);
void cursor_advance(cursor_t *c, uint32_t dt_ms);
const timeline_entry_t *cursor_current(const cursor_t *c);

/* Boots the director task. Returns once the task is running. */
int  director_start(const timeline_entry_t *entries, int n);
```

- [ ] **Step 3: Failing tests for the cursor**

`test_host/test_director.c`:
```c
#include "unity.h"
#include "director.h"
#include "scene.h"

static const scene_t SCENE_A = { .name = "A", .est_duration_ms = 0 };
static const scene_t SCENE_B = { .name = "B", .est_duration_ms = 0 };

void setUp(void) {}
void tearDown(void) {}

void test_cursor_starts_at_first_entry(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 1000 },
        { .scene = &SCENE_B, .duration_ms = 2000 },
    };
    cursor_t c;
    cursor_init(&c, e, 2);
    TEST_ASSERT_EQUAL_PTR(&SCENE_A, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(0, c.t_in_scene_ms);
}

void test_cursor_advances_within_scene(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 1000 },
        { .scene = &SCENE_B, .duration_ms = 2000 },
    };
    cursor_t c;
    cursor_init(&c, e, 2);
    cursor_advance(&c, 500);
    TEST_ASSERT_EQUAL_PTR(&SCENE_A, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(500, c.t_in_scene_ms);
}

void test_cursor_swaps_scene_on_overflow(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 1000 },
        { .scene = &SCENE_B, .duration_ms = 2000 },
    };
    cursor_t c;
    cursor_init(&c, e, 2);
    cursor_advance(&c, 1200);  /* crosses A->B */
    TEST_ASSERT_EQUAL_PTR(&SCENE_B, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(200, c.t_in_scene_ms);
}

void test_cursor_loops_at_end(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 1000 },
        { .scene = &SCENE_B, .duration_ms = 2000 },
    };
    cursor_t c;
    cursor_init(&c, e, 2);
    cursor_advance(&c, 3500);  /* past end of B (1000+2000), wraps */
    TEST_ASSERT_EQUAL_PTR(&SCENE_A, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(500, c.t_in_scene_ms);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cursor_starts_at_first_entry);
    RUN_TEST(test_cursor_advances_within_scene);
    RUN_TEST(test_cursor_swaps_scene_on_overflow);
    RUN_TEST(test_cursor_loops_at_end);
    return UNITY_END();
}
```

Add to `test_host/CMakeLists.txt`:
```cmake
add_executable(test_director test_director.c ../components/director/director.c)
target_link_libraries(test_director unity)
add_test(NAME director COMMAND test_director)
```

- [ ] **Step 4: Run, expect link failure**

Run: `./test_host/run.sh`
Expected: link fails — `director.c` missing.

- [ ] **Step 5: Implement the director**

`components/director/director.c`:
```c
#include "director.h"
#include "fb.h"

#ifdef ESP_PLATFORM
#  include "freertos/FreeRTOS.h"
#  include "freertos/task.h"
#  include "esp_timer.h"
#  include "esp_log.h"
#  include <stdlib.h>
static const char *TAG = "director";
#else
#  include <stdlib.h>
#endif

void cursor_init(cursor_t *c, const timeline_entry_t *e, int n)
{
    c->entries = e;
    c->n_entries = n;
    c->idx = 0;
    c->t_in_scene_ms = 0;
}

void cursor_advance(cursor_t *c, uint32_t dt_ms)
{
    c->t_in_scene_ms += dt_ms;
    while (c->t_in_scene_ms >= c->entries[c->idx].duration_ms) {
        c->t_in_scene_ms -= c->entries[c->idx].duration_ms;
        c->idx = (c->idx + 1) % c->n_entries;
    }
}

const timeline_entry_t *cursor_current(const cursor_t *c)
{
    return &c->entries[c->idx];
}

#ifdef ESP_PLATFORM

#define FRAME_INTERVAL_MS 33  /* ≈30 fps target */

typedef struct {
    cursor_t cursor;
    const scene_t *active;
    void *ctx;
} director_state_t;

static void enter_scene(director_state_t *s, const scene_t *sc)
{
    if (s->active) {
        if (s->active->teardown) s->active->teardown(s->ctx);
        free(s->ctx);
        s->ctx = NULL;
    }
    s->active = sc;
    if (sc->ctx_size) {
        s->ctx = calloc(1, sc->ctx_size);
        if (!s->ctx) { ESP_LOGE(TAG, "ctx alloc failed for %s", sc->name); abort(); }
    }
    if (sc->init && sc->init(s->ctx) != 0) {
        ESP_LOGE(TAG, "scene %s init failed", sc->name);
        abort();
    }
    ESP_LOGI(TAG, "→ scene %s (%u ms)", sc->name, (unsigned)sc->est_duration_ms);
}

static void director_task(void *arg)
{
    director_state_t *s = arg;
    enter_scene(s, cursor_current(&s->cursor)->scene);

    int64_t prev = esp_timer_get_time();
    for (;;) {
        int64_t now = esp_timer_get_time();
        uint32_t dt = (uint32_t)((now - prev) / 1000);
        prev = now;

        const scene_t *want = cursor_current(&s->cursor)->scene;
        if (want != s->active) enter_scene(s, want);

        fb_t *fb = fb_acquire_back();
        s->active->render(s->ctx, fb, s->cursor.t_in_scene_ms);
        fb_present();

        cursor_advance(&s->cursor, dt);

        TickType_t target = pdMS_TO_TICKS(FRAME_INTERVAL_MS);
        vTaskDelay(target);
    }
}

int director_start(const timeline_entry_t *entries, int n)
{
    static director_state_t s_state;
    cursor_init(&s_state.cursor, entries, n);
    if (xTaskCreate(director_task, "director", 8192, &s_state, 5, NULL) != pdPASS)
        return -1;
    return 0;
}

#endif  /* ESP_PLATFORM */
```

`components/director/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "director.c"
    INCLUDE_DIRS "include"
    REQUIRES fb esp_timer freertos esp_common log)
```

- [ ] **Step 6: Run host tests, expect green**

Run: `./test_host/run.sh`
Expected: 4/4 director cursor tests pass.

- [ ] **Step 7: Wire in app_main with a one-scene "purple" timeline as on-target smoke**

Modify `main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES lcd_drv fb gfx director)
```

Replace `main/app_main.c`:
```c
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_drv.h"
#include "fb.h"
#include "gfx.h"
#include "director.h"
#include "scene.h"

static const char *TAG = "esp32demo";

static void purple_render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    /* Pulse purple intensity with t, just to prove the loop runs. */
    uint8_t v = (uint8_t)(128 + 127 * (int)((t / 20) % 2));
    gfx_clear(fb, fb_rgb565(v, 0, v));
}

static const scene_t SCENE_PURPLE = {
    .name = "purple", .est_duration_ms = 5000,
    .render = purple_render,
};

static const timeline_entry_t timeline[] = {
    { .scene = &SCENE_PURPLE, .duration_ms = 5000 },
};

void app_main(void)
{
    ESP_LOGI(TAG, "boot");
    ESP_ERROR_CHECK(lcd_drv_init());
    if (fb_init() != 0) abort();
    if (director_start(timeline, 1) != 0) abort();
    ESP_LOGI(TAG, "director running");
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
```

- [ ] **Step 8: Build, flash, verify**

Run: `idf.py build flash monitor`
Expected: screen pulses purple. Log shows `→ scene purple (5000 ms)`. Frame loop runs continuously.

- [ ] **Step 9: Commit**

```bash
git add components/director test_host/test_director.c test_host/CMakeLists.txt main/
git commit -m "Task 6: director task + scene API + cursor host tests + purple smoke"
```

---

## Task 7: Scene 2 — Copper Bars + Sine Scroller

The first real scene. Cycling palette horizontal bands fill the screen; a sine-distorted scroller drifts across the bottom with the demoscene "GREETZ".

**Files:**
- Create: `components/scenes/include/scenes.h`
- Create: `components/scenes/scene_02_copper_scroller.c`
- Create: `components/scenes/CMakeLists.txt`
- Create: `test_host/test_scene_02.c`
- Modify: `test_host/CMakeLists.txt`
- Modify: `main/app_main.c`, `main/CMakeLists.txt`

- [ ] **Step 1: Public scenes header**

`components/scenes/include/scenes.h`:
```c
#pragma once
#include "scene.h"
extern const scene_t SCENE_02_COPPER_SCROLLER;
extern const scene_t SCENE_04_CUBE;
extern const scene_t SCENE_08_SILHOUETTE;
```

- [ ] **Step 2: Failing test — render a single frame, confirm bands & non-blank scroller area**

`test_host/test_scene_02.c`:
```c
#include "unity.h"
#include "scenes.h"
#include "fb.h"
#include <string.h>
#include <stdlib.h>

extern int ppm_write_rgb565(const char *path, const uint16_t *fb, int w, int h);

static uint16_t s_pixels[FB_W * FB_H];
static fb_t s_fb = { .pixels = s_pixels, .w = FB_W, .h = FB_H };

void setUp(void)    { memset(s_pixels, 0, sizeof(s_pixels)); }
void tearDown(void) {}

void test_scene_02_renders_distinct_band_rows(void) {
    void *ctx = calloc(1, SCENE_02_COPPER_SCROLLER.ctx_size);
    SCENE_02_COPPER_SCROLLER.init(ctx);
    SCENE_02_COPPER_SCROLLER.render(ctx, &s_fb, 0);
    /* Sample two rows that should be in different palette bands. */
    uint16_t row5  = s_pixels[5 * FB_W + 100];
    uint16_t row60 = s_pixels[60 * FB_W + 100];
    TEST_ASSERT_NOT_EQUAL(row5, row60);
    if (SCENE_02_COPPER_SCROLLER.teardown) SCENE_02_COPPER_SCROLLER.teardown(ctx);
    free(ctx);
}

void test_scene_02_scroller_moves_with_time(void) {
    void *ctx = calloc(1, SCENE_02_COPPER_SCROLLER.ctx_size);
    SCENE_02_COPPER_SCROLLER.init(ctx);
    SCENE_02_COPPER_SCROLLER.render(ctx, &s_fb, 0);
    ppm_write_rgb565("/tmp/scene_02_t0.ppm", s_pixels, FB_W, FB_H);

    uint16_t snap_t0[FB_W * FB_H];
    memcpy(snap_t0, s_pixels, sizeof(snap_t0));

    SCENE_02_COPPER_SCROLLER.render(ctx, &s_fb, 2000);
    ppm_write_rgb565("/tmp/scene_02_t2000.ppm", s_pixels, FB_W, FB_H);

    /* Some rows in the scroller band must differ between t=0 and t=2s. */
    int diffs = 0;
    int scroller_y = FB_H - 16;
    for (int x = 0; x < FB_W; ++x) {
        if (s_pixels[scroller_y * FB_W + x] != snap_t0[scroller_y * FB_W + x]) diffs++;
    }
    TEST_ASSERT_TRUE_MESSAGE(diffs > 10, "scroller band did not move with time");

    if (SCENE_02_COPPER_SCROLLER.teardown) SCENE_02_COPPER_SCROLLER.teardown(ctx);
    free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_02_renders_distinct_band_rows);
    RUN_TEST(test_scene_02_scroller_moves_with_time);
    return UNITY_END();
}
```

Add to `test_host/CMakeLists.txt`:
```cmake
add_executable(test_scene_02
    test_scene_02.c
    ../components/scenes/scene_02_copper_scroller.c
    ../components/gfx/gfx.c
    ../components/gfx/font_5x7.c)
target_link_libraries(test_scene_02 unity ppm m)
add_test(NAME scene_02 COMMAND test_scene_02)
```

- [ ] **Step 3: Run, expect link failure**

Run: `./test_host/run.sh`
Expected: link fails — scene file doesn't exist.

- [ ] **Step 4: Implement Scene 2**

`components/scenes/scene_02_copper_scroller.c`:
```c
#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include <math.h>
#include <string.h>

#define BAND_H        4
#define SCROLLER_Y    (FB_H - 16)
#define SCROLLER_AMP  6
#define SCROLLER_TEXT ">>> GREETZ TO SPACEBALLS - SCOOPEX - KEFRENS - SANITY - ANARCHY - FUTURE CREW - TBL <<< "

typedef struct { int dummy; } scene_02_ctx_t;

static int s_init(void *ctx) { (void)ctx; return 0; }

static uint16_t copper_color(int row, uint32_t t)
{
    /* Rotate hue across rows; cycle slowly with t. */
    float h = (float)row / FB_H + (t / 4000.0f);
    h = h - (int)h;
    /* HSV-ish with V=1, S=1 — cheap approximation. */
    int seg = (int)(h * 6.0f);
    float f = h * 6.0f - seg;
    uint8_t v = 255;
    uint8_t p = 0;
    uint8_t q = (uint8_t)((1.0f - f) * 255);
    uint8_t r = (uint8_t)(f * 255);
    switch (seg % 6) {
        case 0: return fb_rgb565(v, r, p);
        case 1: return fb_rgb565(q, v, p);
        case 2: return fb_rgb565(p, v, r);
        case 3: return fb_rgb565(p, q, v);
        case 4: return fb_rgb565(r, p, v);
        default:return fb_rgb565(v, p, q);
    }
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    /* Bands. */
    for (int y = 0; y < FB_H; ++y) {
        int band = y / BAND_H;
        uint16_t c = copper_color(band, t);
        gfx_hline(fb, 0, y, FB_W, c);
    }
    /* Black strip behind scroller for legibility. */
    for (int y = SCROLLER_Y - 2; y < SCROLLER_Y + 9; ++y) {
        gfx_hline(fb, 0, y, FB_W, fb_rgb565(0, 0, 0));
    }
    /* Scroller text — shifted by t, sine-distorted vertically. */
    int n = (int)strlen(SCROLLER_TEXT);
    int pixels_per_char = 6;
    int total_w = n * pixels_per_char;
    int shift = (int)((t / 30) % total_w);
    for (int i = 0; i < n; ++i) {
        int cx = i * pixels_per_char - shift;
        /* Wrap the text horizontally so it scrolls infinitely. */
        cx = ((cx % total_w) + total_w) % total_w;
        cx -= 32;
        if (cx > FB_W) continue;
        if (cx < -pixels_per_char) continue;
        float phase = (cx + t * 0.05f) * 0.05f;
        int dy = (int)(SCROLLER_AMP * sinf(phase));
        char one[2] = { SCROLLER_TEXT[i], 0 };
        gfx_text_5x7(fb, cx, SCROLLER_Y + dy, one, fb_rgb565(255, 255, 255));
    }
}

const scene_t SCENE_02_COPPER_SCROLLER = {
    .name = "copper_scroller",
    .est_duration_ms = 30000,
    .init = s_init,
    .render = render,
    .ctx_size = sizeof(scene_02_ctx_t),
};
```

`components/scenes/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "scene_02_copper_scroller.c"
    INCLUDE_DIRS "include"
    REQUIRES director gfx fb)
```

- [ ] **Step 5: Run host tests**

Run: `./test_host/run.sh`
Expected: scene_02 tests pass. Inspect `/tmp/scene_02_t0.ppm` and `/tmp/scene_02_t2000.ppm` — should look like rainbow bands with white scrolling text on a black strip.

- [ ] **Step 6: Wire scene 2 into app_main as the only scene**

Modify `main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES lcd_drv fb gfx director scenes)
```

Replace the timeline in `main/app_main.c`:
```c
#include "scenes.h"
/* ... keep existing includes, drop the purple_render bits ... */

static const timeline_entry_t timeline[] = {
    { .scene = &SCENE_02_COPPER_SCROLLER, .duration_ms = 30000 },
};

void app_main(void)
{
    ESP_LOGI(TAG, "boot");
    ESP_ERROR_CHECK(lcd_drv_init());
    if (fb_init() != 0) abort();
    if (director_start(timeline, 1) != 0) abort();
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
```

- [ ] **Step 7: Build, flash, verify by eye**

Run: `idf.py build flash monitor`
Expected: rainbow copper bars cycling, scroller crawls at the bottom with sine wave, GREETZ visible. Loop continues indefinitely (timeline of one entry → wraps to itself).

- [ ] **Step 8: Commit**

```bash
git add components/scenes/include components/scenes/scene_02_copper_scroller.c components/scenes/CMakeLists.txt test_host/test_scene_02.c test_host/CMakeLists.txt main/
git commit -m "Task 7: Scene 2 (copper bars + sine scroller) + host snapshot tests"
```

---

## Task 8: Scene 4 (cube only) — wireframe rotating cube

Just the cube portion of the Vector Parade. The full parade (cylinder, polyhedra, X-Wing/TIE) is Phase 2.

**Files:**
- Create: `components/scenes/scene_04_cube.c`
- Create: `test_host/test_scene_04.c`
- Modify: `components/scenes/include/scenes.h` (already declared)
- Modify: `components/scenes/CMakeLists.txt`
- Modify: `test_host/CMakeLists.txt`

- [ ] **Step 1: Failing test — cube renders ≥12 lit pixels somewhere**

`test_host/test_scene_04.c`:
```c
#include "unity.h"
#include "scenes.h"
#include "fb.h"
#include <string.h>
#include <stdlib.h>

extern int ppm_write_rgb565(const char *path, const uint16_t *fb, int w, int h);

static uint16_t s_pixels[FB_W * FB_H];
static fb_t s_fb = { .pixels = s_pixels, .w = FB_W, .h = FB_H };

void setUp(void)    { memset(s_pixels, 0, sizeof(s_pixels)); }
void tearDown(void) {}

void test_scene_04_cube_draws_lines(void) {
    void *ctx = calloc(1, SCENE_04_CUBE.ctx_size);
    SCENE_04_CUBE.init(ctx);
    SCENE_04_CUBE.render(ctx, &s_fb, 1000);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    TEST_ASSERT_TRUE_MESSAGE(set >= 12, "cube must light at least a dozen pixels");
    ppm_write_rgb565("/tmp/scene_04_cube.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_04_CUBE.teardown) SCENE_04_CUBE.teardown(ctx);
    free(ctx);
}

void test_scene_04_cube_animates(void) {
    void *ctx = calloc(1, SCENE_04_CUBE.ctx_size);
    SCENE_04_CUBE.init(ctx);
    SCENE_04_CUBE.render(ctx, &s_fb, 0);
    uint16_t snap[FB_W * FB_H];
    memcpy(snap, s_pixels, sizeof(snap));
    memset(s_pixels, 0, sizeof(s_pixels));
    SCENE_04_CUBE.render(ctx, &s_fb, 500);
    int diffs = 0;
    for (int i = 0; i < FB_W * FB_H; ++i)
        if (s_pixels[i] != snap[i]) diffs++;
    TEST_ASSERT_TRUE_MESSAGE(diffs > 4, "cube did not move between frames");
    if (SCENE_04_CUBE.teardown) SCENE_04_CUBE.teardown(ctx);
    free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_04_cube_draws_lines);
    RUN_TEST(test_scene_04_cube_animates);
    return UNITY_END();
}
```

Add to `test_host/CMakeLists.txt`:
```cmake
add_executable(test_scene_04
    test_scene_04.c
    ../components/scenes/scene_04_cube.c
    ../components/gfx/gfx.c
    ../components/gfx/font_5x7.c
    ../components/gfx3d/gfx3d.c)
target_link_libraries(test_scene_04 unity ppm m)
add_test(NAME scene_04 COMMAND test_scene_04)
```

- [ ] **Step 2: Run, expect link failure**

Run: `./test_host/run.sh`
Expected: link fails — `scene_04_cube.c` missing.

- [ ] **Step 3: Implement Scene 4 cube**

`components/scenes/scene_04_cube.c`:
```c
#include "scenes.h"
#include "gfx.h"
#include "gfx3d.h"
#include <math.h>

static const vec3_t s_cube_verts[8] = {
    {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1},
};

static const uint16_t s_cube_edges[] = {
    0,1, 1,2, 2,3, 3,0,         /* back face */
    4,5, 5,6, 6,7, 7,4,         /* front face */
    0,4, 1,5, 2,6, 3,7,         /* connecting */
};

static const model_t s_cube = {
    .verts = s_cube_verts, .edges = s_cube_edges,
    .n_edges = sizeof(s_cube_edges) / (sizeof(uint16_t) * 2),
};

typedef struct { int dummy; } scene_04_ctx_t;

static int s_init(void *ctx) { (void)ctx; return 0; }

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    gfx_clear(fb, fb_rgb565(0, 0, 16));

    float rt = t / 1000.0f;
    mat4_t world = mat4_translation(0, 0, -5);
    world = mat4_mul(world, mat4_rotation_y(rt));
    world = mat4_mul(world, mat4_rotation_x(rt * 0.7f));

    mat4_t proj = mat4_perspective((float)M_PI / 3.0f,
                                   (float)FB_W / FB_H, 0.1f, 100.0f);

    gfx3d_draw_wireframe(fb, &s_cube, world, proj, fb_rgb565(0, 255, 255));
}

const scene_t SCENE_04_CUBE = {
    .name = "cube",
    .est_duration_ms = 8000,
    .init = s_init,
    .render = render,
    .ctx_size = sizeof(scene_04_ctx_t),
};
```

Update `components/scenes/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "scene_02_copper_scroller.c" "scene_04_cube.c"
    INCLUDE_DIRS "include"
    REQUIRES director gfx gfx3d fb)
```

- [ ] **Step 4: Run host tests**

Run: `./test_host/run.sh`
Expected: scene_04 tests pass. Inspect `/tmp/scene_04_cube.ppm` — cyan wireframe cube on dark blue background.

- [ ] **Step 5: Add cube to the timeline as standalone smoke**

Replace the timeline in `main/app_main.c`:
```c
static const timeline_entry_t timeline[] = {
    { .scene = &SCENE_04_CUBE, .duration_ms = 8000 },
};
```

- [ ] **Step 6: Build, flash, verify by eye**

Run: `idf.py build flash monitor`
Expected: rotating cyan wireframe cube on dark background.

- [ ] **Step 7: Commit**

```bash
git add components/scenes/scene_04_cube.c components/scenes/CMakeLists.txt test_host/test_scene_04.c test_host/CMakeLists.txt main/app_main.c
git commit -m "Task 8: Scene 4 (cube only) wireframe + host snapshot tests"
```

---

## Task 9: Two-scene timeline + FADE_BLACK transition

Both scenes (2 and 4) into one timeline; add the universal black-fade transition between them.

**Files:**
- Create: `components/director/transitions.c`
- Modify: `components/director/include/scene.h`
- Modify: `components/director/include/director.h`
- Modify: `components/director/director.c`
- Modify: `components/director/CMakeLists.txt`
- Modify: `main/app_main.c`
- Create: `main/timeline.c`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Add transition kinds to the scene/director headers**

Append to `components/director/include/scene.h`:
```c
typedef enum {
    TRANSITION_NONE = 0,
    TRANSITION_FADE_BLACK,
    /* TRANSITION_PALETTE_WARP, TRANSITION_HYPERSPACE — added in Phase 2 */
} transition_kind_t;
```

(`timeline_entry_t.transition` already exists from Task 6 — change its type from `int` to `transition_kind_t`.)

- [ ] **Step 2: Implement the fade-to-black transition**

`components/director/transitions.c`:
```c
#include "scene.h"
#include "fb.h"
#include "gfx.h"
#include <stdint.h>

/* Apply a uniform black darken to the existing fb based on progress 0..1.
 * 0 = no change, 1 = fully black. */
static void darken(fb_t *fb, float k)
{
    if (k <= 0) return;
    uint8_t scale = (uint8_t)((1.0f - k) * 32);  /* clamp to 5-bit channels */
    for (int i = 0; i < fb->w * fb->h; ++i) {
        uint16_t p = fb->pixels[i];
        uint8_t r = (p >> 11) & 0x1F;
        uint8_t g = (p >> 5)  & 0x3F;
        uint8_t b = (p)       & 0x1F;
        r = (uint8_t)(r * scale / 32);
        g = (uint8_t)(g * (scale * 2) / 64);
        b = (uint8_t)(b * scale / 32);
        fb->pixels[i] = (r << 11) | (g << 5) | b;
    }
}

void transition_fade_black_apply(fb_t *fb, uint32_t t_in_transition_ms,
                                 uint32_t total_ms)
{
    if (total_ms == 0) return;
    float k = (float)t_in_transition_ms / (float)total_ms;
    if (k > 1.0f) k = 1.0f;
    darken(fb, k);
}
```

Append to `components/director/include/director.h`:
```c
void transition_fade_black_apply(fb_t *fb, uint32_t t_in_transition_ms,
                                 uint32_t total_ms);
```

- [ ] **Step 3: Update the director task to honor transition entries**

In `components/director/director.c` `director_task`, replace the per-frame body:
```c
const timeline_entry_t *cur = cursor_current(&s->cursor);

if (cur->scene == NULL) {
    /* Transition entry — render a frame of the *previous* scene then darken. */
    fb_t *fb = fb_acquire_back();
    if (s->active && s->active->render)
        s->active->render(s->ctx, fb, s->cursor.t_in_scene_ms);
    if (cur->transition == TRANSITION_FADE_BLACK)
        transition_fade_black_apply(fb, s->cursor.t_in_scene_ms, cur->duration_ms);
    fb_present();
} else {
    if (cur->scene != s->active) enter_scene(s, cur->scene);
    fb_t *fb = fb_acquire_back();
    s->active->render(s->ctx, fb, s->cursor.t_in_scene_ms);
    fb_present();
}

cursor_advance(&s->cursor, dt);
```

Update `components/director/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "director.c" "transitions.c"
    INCLUDE_DIRS "include"
    REQUIRES fb gfx esp_timer freertos esp_common log)
```

- [ ] **Step 4: Move the timeline out of `app_main.c` into `main/timeline.c`**

`main/timeline.c`:
```c
#include "scene.h"
#include "scenes.h"

const timeline_entry_t TIMELINE[] = {
    { .scene = &SCENE_02_COPPER_SCROLLER, .duration_ms = 30000 },
    { .transition = TRANSITION_FADE_BLACK,                   .duration_ms = 500 },
    { .scene = &SCENE_04_CUBE,             .duration_ms = 8000 },
    { .transition = TRANSITION_FADE_BLACK,                   .duration_ms = 500 },
};

const int TIMELINE_LEN = sizeof(TIMELINE) / sizeof(TIMELINE[0]);
```

`main/app_main.c`:
```c
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_drv.h"
#include "fb.h"
#include "director.h"
#include "scene.h"

extern const timeline_entry_t TIMELINE[];
extern const int               TIMELINE_LEN;

static const char *TAG = "esp32demo";

void app_main(void)
{
    ESP_LOGI(TAG, "boot — Phase 1, 2 scenes");
    ESP_ERROR_CHECK(lcd_drv_init());
    if (fb_init() != 0) abort();
    if (director_start(TIMELINE, TIMELINE_LEN) != 0) abort();
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
```

`main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "app_main.c" "timeline.c"
    INCLUDE_DIRS "."
    REQUIRES lcd_drv fb gfx director scenes)
```

- [ ] **Step 5: Build, flash, verify by eye**

Run: `idf.py build flash monitor`
Expected: copper-bar/scroller scene plays for 30 s, fades to black over 0.5 s, cube spins for 8 s, fades, loops. Logs show `→ scene copper_scroller` then `→ scene cube` repeatedly.

- [ ] **Step 6: Commit**

```bash
git add components/director main/
git commit -m "Task 9: two-scene timeline + FADE_BLACK transition"
```

---

## Task 10: SD prefetch (`prefetch`) component

Mounts the SD card, exposes a tiny "ring buffer of frames" abstraction. Pure-pool logic TDD'd on host; mount + read paths smoke-tested on hardware.

**Files:**
- Create: `components/prefetch/include/prefetch.h`
- Create: `components/prefetch/prefetch.c`
- Create: `components/prefetch/CMakeLists.txt`
- Create: `test_host/test_prefetch.c`
- Modify: `test_host/CMakeLists.txt`

- [ ] **Step 0: Confirm SD pinout for ESP32-C6-Geek**

Open the board schematic from
`https://www.waveshare.com/wiki/ESP32-C6-GEEK` (look for the schematic PDF
linked under Resources). Confirm which GPIOs the TF slot uses (CMD, CLK, D0)
and that they are free of conflict with the LCD pins (1, 2, 3, 4, 5, 6).

Record findings as comments in `components/prefetch/include/prefetch.h`
under `SD_PIN_*` defines. **Do not guess** — if the schematic isn't
accessible, surface this as a blocker before continuing.

- [ ] **Step 1: Public API**

`components/prefetch/include/prefetch.h`:
```c
#pragma once
#include <stdint.h>
#include <stddef.h>

/* SD pin assignments — fill in from the board schematic in Step 0. */
#define SD_PIN_CMD   /* TBD — fill from schematic */ -1
#define SD_PIN_CLK   /* TBD */                       -1
#define SD_PIN_D0    /* TBD */                       -1
#define SD_PIN_CS    /* TBD if SPI mode */           -1

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
```

- [ ] **Step 2: Failing host tests for the pure ring-state logic**

`test_host/test_prefetch.c`:
```c
#include "unity.h"
#include "prefetch.h"

void setUp(void) {}
void tearDown(void) {}

void test_ring_state_init(void) {
    ring_state_t s;
    ring_state_init(&s, 120, 4);
    TEST_ASSERT_EQUAL_INT(120, s.total);
    TEST_ASSERT_EQUAL_INT(4, s.capacity);
    TEST_ASSERT_EQUAL_INT(0, s.next_load);
    TEST_ASSERT_EQUAL_INT(0, s.oldest_loaded);
}

void test_ring_evicts_oldest_when_full(void) {
    ring_state_t s;
    ring_state_init(&s, 120, 4);
    /* Pretend frames 0..3 are loaded; need frame 4. Evict 0. */
    s.oldest_loaded = 0;
    s.next_load = 4;
    int victim = ring_state_next_evict(&s, 4);
    TEST_ASSERT_EQUAL_INT(0, victim);
}

void test_ring_eviction_for_random_access_picks_furthest_from_request(void) {
    ring_state_t s;
    ring_state_init(&s, 120, 4);
    s.oldest_loaded = 10;
    s.next_load = 14;
    /* Caller jumps backward to frame 9: evict the *furthest* loaded
     * (frame 13) since frames close to the request are about to be needed. */
    int victim = ring_state_next_evict(&s, 9);
    TEST_ASSERT_EQUAL_INT(13, victim);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ring_state_init);
    RUN_TEST(test_ring_evicts_oldest_when_full);
    RUN_TEST(test_ring_eviction_for_random_access_picks_furthest_from_request);
    return UNITY_END();
}
```

Add to `test_host/CMakeLists.txt`:
```cmake
add_executable(test_prefetch test_prefetch.c ../components/prefetch/prefetch.c)
target_compile_definitions(test_prefetch PRIVATE PREFETCH_HOST_TEST=1)
target_link_libraries(test_prefetch unity)
add_test(NAME prefetch COMMAND test_prefetch)
```

- [ ] **Step 3: Run, expect link failure**

Run: `./test_host/run.sh`
Expected: link fails — `prefetch.c` missing.

- [ ] **Step 4: Implement `prefetch.c`**

`components/prefetch/prefetch.c`:
```c
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

int prefetch_mount_sd(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };
    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_PIN_CS;
    slot_cfg.host_id = SPI3_HOST;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_CMD,
        .miso_io_num = SD_PIN_D0,
        .sclk_io_num = SD_PIN_CLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = 8192,
    };
    if (spi_bus_initialize(SPI3_HOST, &bus_cfg, SDSPI_DEFAULT_DMA) != ESP_OK) return -1;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    sdmmc_card_t *card;
    if (esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_cfg, &mount_cfg, &card) != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed");
        return -1;
    }
    ESP_LOGI(TAG, "SD mounted (size=%lluMB)", ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
    return 0;
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
    char path[80];
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
        /* Evict the slot whose loaded index is furthest from `idx`. */
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
```

`components/prefetch/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "prefetch.c"
    INCLUDE_DIRS "include"
    REQUIRES driver fatfs sdmmc esp_common log)
```

- [ ] **Step 5: Run, expect green**

Run: `./test_host/run.sh`
Expected: 3/3 prefetch ring-state tests pass.

- [ ] **Step 6: On-target smoke — mount SD and list root**

Add a temporary call to `prefetch_mount_sd()` after `lcd_drv_init()` in `app_main`. Build, flash, monitor. Expected: `SD mounted (size=...)` log line. Remove the temporary call once confirmed.

- [ ] **Step 7: Commit**

```bash
git add components/prefetch test_host/test_prefetch.c test_host/CMakeLists.txt
git commit -m "Task 10: SD prefetch component (mount + frame ring) + ring-state host tests"
```

---

## Task 11: Scene 8 — silhouette streamer

Streams 60 silhouette frames from SD using the prefetch ring. Renders each frame as a single-color silhouette over a moody background.

The silhouette frames are 1-bit packed bitmaps, 60 wide × 100 tall — that's
`(60 * 100) / 8 = 750 bytes per frame` (round to 768). 60 frames × 768 B = 45 KB
on SD. We'll generate a placeholder set procedurally for the vertical slice;
authoring real rotoscoped frames is Phase 3.

**Files:**
- Create: `components/scenes/scene_08_silhouette.c`
- Create: `tools/gen_silhouette_placeholder.py`
- Create: `test_host/test_scene_08.c`
- Modify: `components/scenes/CMakeLists.txt`
- Modify: `test_host/CMakeLists.txt`

- [ ] **Step 1: Generate 60 placeholder silhouette frames**

`tools/gen_silhouette_placeholder.py`:
```python
#!/usr/bin/env python3
"""Generate 60 placeholder silhouette frames as 1-bit packed bitmaps.
Output: sdcard/silhouettes/frame_NNN.bin, 60x100, 768 bytes each.
The 'silhouette' is just a bouncing oval — proves the pipeline works
end-to-end. Real rotoscoped frames replace these in Phase 3."""

import math
import os
import struct

W, H, FRAMES = 60, 100, 60
OUT_DIR = "sdcard/silhouettes"
os.makedirs(OUT_DIR, exist_ok=True)

for f in range(FRAMES):
    bits = bytearray(768)
    cy = int(50 + 30 * math.sin(2 * math.pi * f / FRAMES))
    cx = 30
    for y in range(H):
        for x in range(W):
            inside = ((x - cx) / 18.0) ** 2 + ((y - cy) / 28.0) ** 2 <= 1.0
            if inside:
                byte = (y * W + x) // 8
                bit = (y * W + x) % 8
                bits[byte] |= (1 << bit)
    path = os.path.join(OUT_DIR, f"frame_{f:03d}.bin")
    with open(path, "wb") as out:
        out.write(bits)
print(f"wrote {FRAMES} frames to {OUT_DIR}")
```

```bash
chmod +x tools/gen_silhouette_placeholder.py
./tools/gen_silhouette_placeholder.py
ls sdcard/silhouettes | head -3
```

Expected: 60 files, each 768 B, named `frame_000.bin` through `frame_059.bin`.

- [ ] **Step 2: Failing test — scene 8 lights pixels at expected times**

`test_host/test_scene_08.c`:
```c
/* This scene uses prefetch's frame_ring, which on host has no SD. We test
 * the *bit unpack + render* portion only by feeding a known-shape buffer.
 * The streaming integration is verified on hardware in Step 5. */
#include "unity.h"
#include "scenes.h"
#include "fb.h"
#include <string.h>
#include <stdlib.h>

extern int ppm_write_rgb565(const char *path, const uint16_t *fb, int w, int h);
extern void scene_08_render_with_buffer(fb_t *fb, const void *frame_bits,
                                        int w, int h, uint16_t color);

static uint16_t s_pixels[FB_W * FB_H];
static fb_t s_fb = { .pixels = s_pixels, .w = FB_W, .h = FB_H };

void setUp(void) { memset(s_pixels, 0, sizeof(s_pixels)); }
void tearDown(void) {}

void test_silhouette_renders_solid_block_for_all_set_bits(void) {
    /* 8x8 buffer, all bits set. */
    uint8_t buf[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    scene_08_render_with_buffer(&s_fb, buf, 8, 8, 0xF800);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i] == 0xF800) set++;
    TEST_ASSERT_EQUAL_INT(64, set);
    ppm_write_rgb565("/tmp/scene_08_solid.ppm", s_pixels, FB_W, FB_H);
}

void test_silhouette_skips_unset_bits(void) {
    uint8_t buf[8] = {0};   /* all unset */
    scene_08_render_with_buffer(&s_fb, buf, 8, 8, 0xF800);
    for (int i = 0; i < FB_W * FB_H; ++i) TEST_ASSERT_EQUAL_HEX16(0, s_pixels[i]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_silhouette_renders_solid_block_for_all_set_bits);
    RUN_TEST(test_silhouette_skips_unset_bits);
    return UNITY_END();
}
```

Add to `test_host/CMakeLists.txt`:
```cmake
add_executable(test_scene_08
    test_scene_08.c
    ../components/scenes/scene_08_silhouette.c)
target_compile_definitions(test_scene_08 PRIVATE SCENE_08_HOST_TEST=1)
target_link_libraries(test_scene_08 unity ppm)
add_test(NAME scene_08 COMMAND test_scene_08)
```

- [ ] **Step 3: Run, expect link failure**

Run: `./test_host/run.sh`
Expected: link fails — `scene_08_silhouette.c` missing.

- [ ] **Step 4: Implement Scene 8 (host-testable render path + on-target streaming wrapper)**

`components/scenes/scene_08_silhouette.c`:
```c
#include "scenes.h"
#include "fb.h"
#include <string.h>

#define SIL_W      60
#define SIL_H      100
#define SIL_FRAMES 60
#define SIL_FPS    20

/* Center the silhouette at (FB_W/2, FB_H/2). */
#define SIL_X0     ((FB_W - SIL_W) / 2)
#define SIL_Y0     ((FB_H - SIL_H) / 2)

#ifndef SCENE_08_HOST_TEST
#  include "prefetch.h"
typedef struct { frame_ring_t *ring; } scene_08_ctx_t;
#else
typedef struct { int dummy; } scene_08_ctx_t;
#endif

void scene_08_render_with_buffer(fb_t *fb, const void *bits, int w, int h, uint16_t color)
{
    if (!bits) return;
    const uint8_t *p = bits;
    int x0 = (fb->w - w) / 2;
    int y0 = (fb->h - h) / 2;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int bit_idx = y * w + x;
            if (p[bit_idx >> 3] & (1 << (bit_idx & 7))) {
                int fx = x0 + x, fy = y0 + y;
                if (fx >= 0 && fx < fb->w && fy >= 0 && fy < fb->h)
                    fb->pixels[fy * fb->w + fx] = color;
            }
        }
    }
}

#ifndef SCENE_08_HOST_TEST

static int s_init(void *ctx)
{
    scene_08_ctx_t *c = ctx;
    c->ring = frame_ring_open("/sdcard/silhouettes/frame_%03u.bin",
                              SIL_FRAMES, (SIL_W * SIL_H + 7) / 8, 4);
    return c->ring ? 0 : -1;
}

static void s_teardown(void *ctx)
{
    scene_08_ctx_t *c = ctx;
    frame_ring_close(c->ring);
    c->ring = NULL;
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    scene_08_ctx_t *c = ctx;
    /* Moody dark-purple background. */
    for (int i = 0; i < fb->w * fb->h; ++i) fb->pixels[i] = fb_rgb565(20, 0, 40);

    int frame = (int)((t * SIL_FPS / 1000) % SIL_FRAMES);
    const void *bits = frame_ring_get(c->ring, frame);
    /* Magenta accent for v1 — the "color drift" enrichment is Phase 2. */
    scene_08_render_with_buffer(fb, bits, SIL_W, SIL_H, fb_rgb565(255, 80, 200));
}

const scene_t SCENE_08_SILHOUETTE = {
    .name = "silhouette",
    .est_duration_ms = 30000,
    .init = s_init,
    .render = render,
    .teardown = s_teardown,
    .ctx_size = sizeof(scene_08_ctx_t),
};

#else  /* SCENE_08_HOST_TEST — host build does not need the on-device scene_t */

const scene_t SCENE_08_SILHOUETTE = { .name = "silhouette_stub" };

#endif
```

Update `components/scenes/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS "scene_02_copper_scroller.c" "scene_04_cube.c" "scene_08_silhouette.c"
    INCLUDE_DIRS "include"
    REQUIRES director gfx gfx3d fb prefetch)
```

- [ ] **Step 5: Run host tests**

Run: `./test_host/run.sh`
Expected: 2/2 scene_08 tests pass. Inspect `/tmp/scene_08_solid.ppm` — should show a small solid red 8×8 block centered on the framebuffer.

- [ ] **Step 6: Copy frames to physical SD card**

Insert SD card into the host. Run:
```bash
cp -r sdcard/silhouettes /run/media/$USER/<SDLABEL>/
sync
```
(adjust mount path; user may have to mount manually). Eject and insert into the C6-Geek slot.

- [ ] **Step 7: Wire SD mount into app_main**

In `main/app_main.c`, after `fb_init()`:
```c
#include "prefetch.h"
/* ... */
if (prefetch_mount_sd() != 0) {
    ESP_LOGE(TAG, "no SD card — silhouette scene will fail");
    /* Continue anyway; scene 8 will log frame load failures but director won't crash. */
}
```

Update `main/CMakeLists.txt` REQUIRES:
```cmake
REQUIRES lcd_drv fb gfx director scenes prefetch
```

- [ ] **Step 8: Build, flash, smoke-test scene 8 in isolation**

Temporarily set the timeline to just scene 8:
```c
const timeline_entry_t TIMELINE[] = {
    { .scene = &SCENE_08_SILHOUETTE, .duration_ms = 30000 },
};
const int TIMELINE_LEN = 1;
```

Run: `idf.py build flash monitor`
Expected: dark purple background with a magenta oval that oscillates vertically (the placeholder silhouette). Logs show no SD read errors.

- [ ] **Step 9: Commit**

```bash
git add components/scenes/scene_08_silhouette.c components/scenes/CMakeLists.txt tools/ test_host/test_scene_08.c test_host/CMakeLists.txt main/ sdcard/
git commit -m "Task 11: Scene 8 silhouette streamer + placeholder frame generator"
```

---

## Task 12: Three-scene timeline + soak test

Glue everything together. Final timeline that loops through all three scenes with FADE_BLACK transitions, runs continuously.

**Files:**
- Modify: `main/timeline.c`
- Run: 1-hour soak test on hardware

- [ ] **Step 1: Final Phase-1 timeline**

`main/timeline.c`:
```c
#include "scene.h"
#include "scenes.h"

const timeline_entry_t TIMELINE[] = {
    { .scene = &SCENE_02_COPPER_SCROLLER, .duration_ms = 30000 },
    { .transition = TRANSITION_FADE_BLACK,                   .duration_ms = 500 },
    { .scene = &SCENE_04_CUBE,             .duration_ms = 8000 },
    { .transition = TRANSITION_FADE_BLACK,                   .duration_ms = 500 },
    { .scene = &SCENE_08_SILHOUETTE,       .duration_ms = 30000 },
    { .transition = TRANSITION_FADE_BLACK,                   .duration_ms = 500 },
};

const int TIMELINE_LEN = sizeof(TIMELINE) / sizeof(TIMELINE[0]);
```

- [ ] **Step 2: Build, flash, watch one full loop**

Run: `idf.py build flash monitor`
Expected: copper scroller (30 s) → fade → cube (8 s) → fade → silhouette (30 s) → fade → loops to copper. Logs print scene transitions.

- [ ] **Step 3: Soak test — 1 hour continuous**

Leave the device running for ≥ 1 hour. Periodically (every 15 min) check
the log for:

- Frame rate didn't degrade (no `vTaskDelay underrun` warnings)
- Free heap stable (compare `esp_get_free_heap_size()` at minute 1 vs minute 60)
- No crashes / restarts (no boot banner re-appearing)

If heap declines >2 KB/hour → leak in scene transitions; investigate
`teardown()` paths.
If any scene drops below 24 fps for sustained periods → log it as a
follow-up item for Phase 3 polish.

- [ ] **Step 4: Tag the milestone**

```bash
git tag phase-1-vertical-slice
git log --oneline | head -15
```

- [ ] **Step 5: Final commit**

```bash
git add main/timeline.c
git commit -m "Task 12: Phase-1 final 3-scene timeline + soak-test passes"
```

---

## Phase 1 Done

After Task 12, the device boots into a 70-second loop of three scenes that
together exercise: 2D primitives, 3D pipeline, ping-pong framebuffer, DMA
blit, asset streaming from SD, scene lifecycle, and inter-scene transitions.

**What's left (Phase 2 / Phase 3, separate plans):**

- Scenes 1, 3, 5, 6, 7, 9, 10, 11, 12 (~one task each, similar shape to Tasks 7, 8, 11)
- Vector parade expansion: cylinder, polyhedra, X-Wing, TIE Fighter, Death Star
- Diegetic transitions: HYPERSPACE (3→4), CRYSTALLIZE (intra-scene 6), LIGHT_BURST (intra-scene 10→11)
- Real rotoscoped silhouette frames replacing the placeholder oval
- Beat-timing pass across all scenes
- Per-scene fps tuning where Phase 1 surfaced budget pressure
- **Cross-scene background prefetch.** Spec §3.4 calls for the prefetch task
  to load scene N+1's assets *while N is still rendering*. Phase 1's
  prefetch loads in the scene's `init()` (runs at scene swap, briefly
  blocking). For the silhouette scene's tiny frames this is invisible; for
  the Phase 2 voxel heightmap (~128 KB) it would cause a visible stutter,
  so the cross-scene background load lands in the Phase 2 plan.
