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

void test_scene_09_renders_landscape_at_t1000(void) {
    void *ctx = SCENE_09_VOXEL.ctx_size
        ? calloc(1, SCENE_09_VOXEL.ctx_size) : NULL;
    if (SCENE_09_VOXEL.init) SCENE_09_VOXEL.init(ctx);
    SCENE_09_VOXEL.render(ctx, &s_fb, 1000);
    /* Scene fills the whole screen (sky + terrain). */
    int non_zero = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) non_zero++;
    TEST_ASSERT_TRUE_MESSAGE(non_zero >= FB_W * FB_H * 9 / 10,
                             "voxel scene should fill nearly all pixels");
    ppm_write_rgb565("/tmp/scene_09_t1000.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_09_VOXEL.teardown) SCENE_09_VOXEL.teardown(ctx);
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_09_renders_landscape_at_t1000);
    return UNITY_END();
}
