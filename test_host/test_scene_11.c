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

void test_scene_11_synthwave_renders_at_t5000(void) {
    void *ctx = SCENE_11_SYNTHWAVE.ctx_size
        ? calloc(1, SCENE_11_SYNTHWAVE.ctx_size) : NULL;
    if (SCENE_11_SYNTHWAVE.init) SCENE_11_SYNTHWAVE.init(ctx);
    SCENE_11_SYNTHWAVE.render(ctx, &s_fb, 5000);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    TEST_ASSERT_TRUE_MESSAGE(set >= FB_W * FB_H / 2,
                             "synthwave fills most of the screen with sky+ground");
    ppm_write_rgb565("/tmp/scene_11_t5000.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_11_SYNTHWAVE.teardown) SCENE_11_SYNTHWAVE.teardown(ctx);
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_11_synthwave_renders_at_t5000);
    return UNITY_END();
}
