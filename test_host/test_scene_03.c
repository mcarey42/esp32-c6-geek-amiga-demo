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

void test_scene_03_calm_renders_stars_at_t5000(void) {
    void *ctx = SCENE_03_STARFIELD.ctx_size
        ? calloc(1, SCENE_03_STARFIELD.ctx_size) : NULL;
    if (SCENE_03_STARFIELD.init) SCENE_03_STARFIELD.init(ctx);
    SCENE_03_STARFIELD.render(ctx, &s_fb, 5000);
    /* Count non-bg pixels (bg is rgb565(0,0,8) = 0x0001). */
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i] != 0x0001) set++;
    TEST_ASSERT_TRUE_MESSAGE(set >= 20, "calm phase should show some stars");
    ppm_write_rgb565("/tmp/scene_03_calm.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_03_STARFIELD.teardown) SCENE_03_STARFIELD.teardown(ctx);
    if (ctx) free(ctx);
}

void test_scene_03_hyperspace_at_t13000(void) {
    void *ctx = SCENE_03_STARFIELD.ctx_size
        ? calloc(1, SCENE_03_STARFIELD.ctx_size) : NULL;
    if (SCENE_03_STARFIELD.init) SCENE_03_STARFIELD.init(ctx);
    SCENE_03_STARFIELD.render(ctx, &s_fb, 13000);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i] != 0x0001) set++;
    TEST_ASSERT_TRUE_MESSAGE(set >= 50, "hyperspace should show streaks");
    ppm_write_rgb565("/tmp/scene_03_hyper.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_03_STARFIELD.teardown) SCENE_03_STARFIELD.teardown(ctx);
    if (ctx) free(ctx);
}

void test_scene_03_flash_white_at_t17800(void) {
    void *ctx = SCENE_03_STARFIELD.ctx_size
        ? calloc(1, SCENE_03_STARFIELD.ctx_size) : NULL;
    if (SCENE_03_STARFIELD.init) SCENE_03_STARFIELD.init(ctx);
    SCENE_03_STARFIELD.render(ctx, &s_fb, 17800);
    /* All pixels should be white. */
    int white_count = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i] == 0xFFFF) white_count++;
    TEST_ASSERT_EQUAL_INT(FB_W * FB_H, white_count);
    ppm_write_rgb565("/tmp/scene_03_flash.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_03_STARFIELD.teardown) SCENE_03_STARFIELD.teardown(ctx);
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_03_calm_renders_stars_at_t5000);
    RUN_TEST(test_scene_03_hyperspace_at_t13000);
    RUN_TEST(test_scene_03_flash_white_at_t17800);
    return UNITY_END();
}
