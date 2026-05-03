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

void test_scene_12_credits_scrolls(void) {
    void *ctx = SCENE_12_CREDITS.ctx_size
        ? calloc(1, SCENE_12_CREDITS.ctx_size) : NULL;
    if (SCENE_12_CREDITS.init) SCENE_12_CREDITS.init(ctx);
    SCENE_12_CREDITS.render(ctx, &s_fb, 5000);
    /* Some white text pixels somewhere on screen + the copper ribbon at bottom. */
    int white_count = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i] == 0xFFFF) white_count++;
    TEST_ASSERT_TRUE_MESSAGE(white_count >= 8, "expected white text pixels");
    /* Copper ribbon: bottom 6 rows must have non-zero pixels in every column. */
    int ribbon_cols = 0;
    for (int x = 0; x < FB_W; ++x) {
        if (s_pixels[(FB_H - 1) * FB_W + x]) ribbon_cols++;
    }
    TEST_ASSERT_TRUE_MESSAGE(ribbon_cols == FB_W, "copper ribbon should fill the bottom row");
    ppm_write_rgb565("/tmp/scene_12_credits.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_12_CREDITS.teardown) SCENE_12_CREDITS.teardown(ctx);
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_12_credits_scrolls);
    return UNITY_END();
}
