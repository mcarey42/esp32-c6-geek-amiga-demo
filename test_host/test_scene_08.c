#include "unity.h"
#include "scenes.h"
#include "fb.h"
#include <string.h>
#include <stdlib.h>

extern int ppm_write_rgb565(const char *path, const uint16_t *fb, int w, int h);
extern void scene_08_render_with_buffer(fb_t *fb, const void *frame_bits,
                                        int w, int h, uint16_t color);

static uint16_t s_pixels[FB_W * FB_H];
static fb_t s_fb = { .pixels = s_pixels, .w = FB_W, .h = FB_H };

void setUp(void) { memset(s_pixels, 0, sizeof(s_pixels)); }
void tearDown(void) {}

void test_silhouette_renders_solid_block_for_all_set_bits(void) {
    uint8_t buf[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    scene_08_render_with_buffer(&s_fb, buf, 8, 8, 0xF800);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i] == 0xF800) set++;
    TEST_ASSERT_EQUAL_INT(64, set);
    ppm_write_rgb565("/tmp/scene_08_solid.ppm", s_pixels, FB_W, FB_H);
}

void test_silhouette_skips_unset_bits(void) {
    uint8_t buf[8] = {0};
    scene_08_render_with_buffer(&s_fb, buf, 8, 8, 0xF800);
    for (int i = 0; i < FB_W * FB_H; ++i) TEST_ASSERT_EQUAL_HEX16(0, s_pixels[i]);
}

void test_silhouette_null_buffer_safe(void) {
    scene_08_render_with_buffer(&s_fb, NULL, 8, 8, 0xF800);
    for (int i = 0; i < FB_W * FB_H; ++i) TEST_ASSERT_EQUAL_HEX16(0, s_pixels[i]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_silhouette_renders_solid_block_for_all_set_bits);
    RUN_TEST(test_silhouette_skips_unset_bits);
    RUN_TEST(test_silhouette_null_buffer_safe);
    return UNITY_END();
}
