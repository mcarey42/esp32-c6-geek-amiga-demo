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

void test_scene_10_tunnel_renders_at_t1000(void) {
    void *ctx = SCENE_10_TUNNEL.ctx_size
        ? calloc(1, SCENE_10_TUNNEL.ctx_size) : NULL;
    if (SCENE_10_TUNNEL.init) SCENE_10_TUNNEL.init(ctx);
    SCENE_10_TUNNEL.render(ctx, &s_fb, 1000);
    int non_zero = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) non_zero++;
    TEST_ASSERT_TRUE_MESSAGE(non_zero >= FB_W * FB_H * 9 / 10,
                             "tunnel scene should fill nearly all pixels");
    ppm_write_rgb565("/tmp/scene_10_t1000.ppm", s_pixels, FB_W, FB_H);
    if (SCENE_10_TUNNEL.teardown) SCENE_10_TUNNEL.teardown(ctx);
    if (ctx) free(ctx);
}

void test_scene_10_exit_brightens(void) {
    void *ctx = SCENE_10_TUNNEL.ctx_size
        ? calloc(1, SCENE_10_TUNNEL.ctx_size) : NULL;
    SCENE_10_TUNNEL.render(ctx, &s_fb, 1000);
    long sum_early = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) {
        sum_early += ((s_pixels[i] >> 11) & 0x1F)
                   + ((s_pixels[i] >> 5)  & 0x3F)
                   +  (s_pixels[i]        & 0x1F);
    }
    SCENE_10_TUNNEL.render(ctx, &s_fb, 24500);   /* near end, almost full bright */
    long sum_late = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) {
        sum_late += ((s_pixels[i] >> 11) & 0x1F)
                  + ((s_pixels[i] >> 5)  & 0x3F)
                  +  (s_pixels[i]        & 0x1F);
    }
    ppm_write_rgb565("/tmp/scene_10_exit.ppm", s_pixels, FB_W, FB_H);
    TEST_ASSERT_TRUE_MESSAGE(sum_late > sum_early, "exit-into-light brightens the scene");
    if (ctx) free(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scene_10_tunnel_renders_at_t1000);
    RUN_TEST(test_scene_10_exit_brightens);
    return UNITY_END();
}
