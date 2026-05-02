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
    void *ctx = SCENE_02_COPPER_SCROLLER.ctx_size
        ? calloc(1, SCENE_02_COPPER_SCROLLER.ctx_size) : NULL;
    if (SCENE_02_COPPER_SCROLLER.init) SCENE_02_COPPER_SCROLLER.init(ctx);
    SCENE_02_COPPER_SCROLLER.render(ctx, &s_fb, 0);
    /* Sample two rows that should be in different palette bands. */
    uint16_t row5  = s_pixels[5 * FB_W + 100];
    uint16_t row60 = s_pixels[60 * FB_W + 100];
    TEST_ASSERT_NOT_EQUAL(row5, row60);
    if (SCENE_02_COPPER_SCROLLER.teardown) SCENE_02_COPPER_SCROLLER.teardown(ctx);
    if (ctx) free(ctx);
}

void test_scene_02_scroller_moves_with_time(void) {
    void *ctx = SCENE_02_COPPER_SCROLLER.ctx_size
        ? calloc(1, SCENE_02_COPPER_SCROLLER.ctx_size) : NULL;
    if (SCENE_02_COPPER_SCROLLER.init) SCENE_02_COPPER_SCROLLER.init(ctx);
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
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_02_renders_distinct_band_rows);
    RUN_TEST(test_scene_02_scroller_moves_with_time);
    return UNITY_END();
}
