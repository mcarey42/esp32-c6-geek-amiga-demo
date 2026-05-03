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

void test_scene_07_renders_at_t1000(void) {
    void *ctx = SCENE_07_ROTOZOOMER.ctx_size
        ? calloc(1, SCENE_07_ROTOZOOMER.ctx_size) : NULL;
    if (SCENE_07_ROTOZOOMER.init) SCENE_07_ROTOZOOMER.init(ctx);
    SCENE_07_ROTOZOOMER.render(ctx, &s_fb, 1000);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    TEST_ASSERT_TRUE_MESSAGE(set >= FB_W * FB_H / 2, "rotozoomer should fill most pixels");
    ppm_write_rgb565("/tmp/scene_07_t1000.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_07_ROTOZOOMER.teardown) SCENE_07_ROTOZOOMER.teardown(ctx);
    if (ctx) free(ctx);
}

void test_scene_07_texture_changes(void) {
    /* tex 0 (0..6.5s), tex 1 (6.5..13s), tex 2 (13..20s). */
    void *ctx = SCENE_07_ROTOZOOMER.ctx_size
        ? calloc(1, SCENE_07_ROTOZOOMER.ctx_size) : NULL;
    SCENE_07_ROTOZOOMER.render(ctx, &s_fb, 3000);
    uint16_t snap_t1[FB_W * FB_H];
    memcpy(snap_t1, s_pixels, sizeof(snap_t1));
    SCENE_07_ROTOZOOMER.render(ctx, &s_fb, 16000);
    int diffs = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i] != snap_t1[i]) diffs++;
    TEST_ASSERT_TRUE_MESSAGE(diffs > 1000, "different textures should yield very different output");
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_07_renders_at_t1000);
    RUN_TEST(test_scene_07_texture_changes);
    return UNITY_END();
}
