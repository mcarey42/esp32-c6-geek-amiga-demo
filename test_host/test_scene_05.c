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

void test_scene_05_renders_at_t1000(void) {
    void *ctx = SCENE_05_PLASMA.ctx_size
        ? calloc(1, SCENE_05_PLASMA.ctx_size) : NULL;
    if (SCENE_05_PLASMA.init) SCENE_05_PLASMA.init(ctx);
    SCENE_05_PLASMA.render(ctx, &s_fb, 1000);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    TEST_ASSERT_TRUE_MESSAGE(set >= FB_W * FB_H / 2,
                             "plasma should fill most pixels");
    ppm_write_rgb565("/tmp/scene_05_t1000.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_05_PLASMA.teardown) SCENE_05_PLASMA.teardown(ctx);
    if (ctx) free(ctx);
}

void test_scene_05_palette_walks(void) {
    void *ctx = SCENE_05_PLASMA.ctx_size
        ? calloc(1, SCENE_05_PLASMA.ctx_size) : NULL;
    if (SCENE_05_PLASMA.init) SCENE_05_PLASMA.init(ctx);

    SCENE_05_PLASMA.render(ctx, &s_fb, 1000);
    /* Sum the red channel and the blue channel across the screen. */
    long red_early = 0, blue_early = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) {
        red_early  += (s_pixels[i] >> 11) & 0x1F;
        blue_early += s_pixels[i] & 0x1F;
    }

    SCENE_05_PLASMA.render(ctx, &s_fb, 24000);
    long red_late = 0, blue_late = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) {
        red_late  += (s_pixels[i] >> 11) & 0x1F;
        blue_late += s_pixels[i] & 0x1F;
    }
    ppm_write_rgb565("/tmp/scene_05_t24000.ppm", s_pixels, FB_W, FB_H);

    /* Early frame should be more red, late frame more blue. */
    TEST_ASSERT_TRUE_MESSAGE(red_early > red_late, "red should fall over time");
    TEST_ASSERT_TRUE_MESSAGE(blue_late > blue_early, "blue should rise over time");
    if (SCENE_05_PLASMA.teardown) SCENE_05_PLASMA.teardown(ctx);
    if (ctx) free(ctx);
}

void test_scene_05_breathe_visible_at_midpoint(void) {
    void *ctx = SCENE_05_PLASMA.ctx_size
        ? calloc(1, SCENE_05_PLASMA.ctx_size) : NULL;
    if (SCENE_05_PLASMA.init) SCENE_05_PLASMA.init(ctx);
    SCENE_05_PLASMA.render(ctx, &s_fb, 12500);

    /* The "breathe" word at peak fade-in is white. Look for any near-white
     * pixels in the center band — there must be several. */
    int near_white = 0;
    int row_top = FB_H / 2 - 6;
    int row_bot = FB_H / 2 + 6;
    for (int y = row_top; y < row_bot; ++y) {
        for (int x = 0; x < FB_W; ++x) {
            uint16_t p = s_pixels[y * FB_W + x];
            uint8_t r = (p >> 11) & 0x1F;
            uint8_t g = (p >> 5)  & 0x3F;
            uint8_t b = p & 0x1F;
            /* "white-ish": all channels near max. */
            if (r >= 28 && g >= 56 && b >= 28) near_white++;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(near_white >= 8,
                             "expected white 'breathe' pixels at midpoint");
    ppm_write_rgb565("/tmp/scene_05_breathe.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_05_PLASMA.teardown) SCENE_05_PLASMA.teardown(ctx);
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_05_renders_at_t1000);
    RUN_TEST(test_scene_05_palette_walks);
    RUN_TEST(test_scene_05_breathe_visible_at_midpoint);
    return UNITY_END();
}
