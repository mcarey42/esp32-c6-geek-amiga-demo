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

void test_scene_01_post_renders_text_at_t1000(void) {
    void *ctx = SCENE_01_BOOT_TITLE.ctx_size
        ? calloc(1, SCENE_01_BOOT_TITLE.ctx_size) : NULL;
    if (SCENE_01_BOOT_TITLE.init) SCENE_01_BOOT_TITLE.init(ctx);
    SCENE_01_BOOT_TITLE.render(ctx, &s_fb, 1000);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    TEST_ASSERT_TRUE_MESSAGE(set >= 30, "POST phase should render some text");
    ppm_write_rgb565("/tmp/scene_01_post.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_01_BOOT_TITLE.teardown) SCENE_01_BOOT_TITLE.teardown(ctx);
    if (ctx) free(ctx);
}

void test_scene_01_title_renders_at_t6000(void) {
    void *ctx = SCENE_01_BOOT_TITLE.ctx_size
        ? calloc(1, SCENE_01_BOOT_TITLE.ctx_size) : NULL;
    if (SCENE_01_BOOT_TITLE.init) SCENE_01_BOOT_TITLE.init(ctx);
    SCENE_01_BOOT_TITLE.render(ctx, &s_fb, 6000);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    /* Title has copper bars filling the whole frame plus the chunky logo. */
    TEST_ASSERT_TRUE_MESSAGE(set >= FB_W * FB_H / 2,
                             "Title phase should fill most of the screen");
    ppm_write_rgb565("/tmp/scene_01_title.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_01_BOOT_TITLE.teardown) SCENE_01_BOOT_TITLE.teardown(ctx);
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_01_post_renders_text_at_t1000);
    RUN_TEST(test_scene_01_title_renders_at_t6000);
    return UNITY_END();
}
