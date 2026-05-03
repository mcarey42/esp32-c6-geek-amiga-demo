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

void test_scene_06_renders_blobs_at_t5000(void) {
    void *ctx = SCENE_06_METABALLS.ctx_size
        ? calloc(1, SCENE_06_METABALLS.ctx_size) : NULL;
    if (SCENE_06_METABALLS.init) SCENE_06_METABALLS.init(ctx);
    SCENE_06_METABALLS.render(ctx, &s_fb, 5000);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    TEST_ASSERT_TRUE_MESSAGE(set >= FB_W * FB_H / 4, "metaballs should fill many pixels");
    ppm_write_rgb565("/tmp/scene_06_t5000.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_06_METABALLS.teardown) SCENE_06_METABALLS.teardown(ctx);
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_06_renders_blobs_at_t5000);
    return UNITY_END();
}
