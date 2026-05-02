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
    void *ctx = SCENE_04_CUBE.ctx_size
        ? calloc(1, SCENE_04_CUBE.ctx_size) : NULL;
    if (SCENE_04_CUBE.init) SCENE_04_CUBE.init(ctx);
    SCENE_04_CUBE.render(ctx, &s_fb, 1000);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    TEST_ASSERT_TRUE_MESSAGE(set >= 12, "cube must light at least a dozen pixels");
    ppm_write_rgb565("/tmp/scene_04_cube.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_04_CUBE.teardown) SCENE_04_CUBE.teardown(ctx);
    if (ctx) free(ctx);
}

void test_scene_04_cube_animates(void) {
    void *ctx = SCENE_04_CUBE.ctx_size
        ? calloc(1, SCENE_04_CUBE.ctx_size) : NULL;
    if (SCENE_04_CUBE.init) SCENE_04_CUBE.init(ctx);
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
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_04_cube_draws_lines);
    RUN_TEST(test_scene_04_cube_animates);
    return UNITY_END();
}
