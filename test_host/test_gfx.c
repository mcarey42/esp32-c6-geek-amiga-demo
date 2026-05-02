#include "unity.h"
#include "gfx.h"
#include <stdint.h>
#include <string.h>

extern int ppm_write_rgb565(const char *path, const uint16_t *fb, int w, int h);

static uint16_t s_pixels[FB_W * FB_H];
static fb_t s_fb = { .pixels = s_pixels, .w = FB_W, .h = FB_H };

void setUp(void)    { memset(s_pixels, 0, sizeof(s_pixels)); }
void tearDown(void) {}

void test_clear_fills_all_pixels(void) {
    gfx_clear(&s_fb, 0xABCD);
    for (int i = 0; i < FB_W * FB_H; ++i)
        TEST_ASSERT_EQUAL_HEX16(0xABCD, s_pixels[i]);
}

void test_pixel_writes_single_location(void) {
    gfx_pixel(&s_fb, 10, 20, 0xBEEF);
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, s_pixels[20 * FB_W + 10]);
    TEST_ASSERT_EQUAL_HEX16(0,      s_pixels[20 * FB_W + 11]);
    TEST_ASSERT_EQUAL_HEX16(0,      s_pixels[19 * FB_W + 10]);
}

void test_pixel_clips_oob(void) {
    gfx_pixel(&s_fb, -1,    0, 0xFFFF);
    gfx_pixel(&s_fb, FB_W,  0, 0xFFFF);
    gfx_pixel(&s_fb, 0,    -1, 0xFFFF);
    gfx_pixel(&s_fb, 0, FB_H,  0xFFFF);
    /* No assertion needed — we just must not crash or write OOB. */
}

void test_hline_writes_n_pixels(void) {
    gfx_hline(&s_fb, 5, 10, 7, 0x1234);
    for (int i = 0; i < 7; ++i)
        TEST_ASSERT_EQUAL_HEX16(0x1234, s_pixels[10 * FB_W + 5 + i]);
    TEST_ASSERT_EQUAL_HEX16(0, s_pixels[10 * FB_W + 5 + 7]);
}

void test_hline_clips_at_right_edge(void) {
    gfx_hline(&s_fb, FB_W - 3, 0, 100, 0xAAAA);
    TEST_ASSERT_EQUAL_HEX16(0xAAAA, s_pixels[FB_W - 3]);
    TEST_ASSERT_EQUAL_HEX16(0xAAAA, s_pixels[FB_W - 1]);
    /* y=1 row 0 must remain untouched */
    TEST_ASSERT_EQUAL_HEX16(0, s_pixels[FB_W]);
}

void test_vline_writes_n_pixels(void) {
    gfx_vline(&s_fb, 50, 10, 5, 0x5678);
    for (int i = 0; i < 5; ++i)
        TEST_ASSERT_EQUAL_HEX16(0x5678, s_pixels[(10 + i) * FB_W + 50]);
}

void test_line_diagonal_endpoints(void) {
    gfx_line(&s_fb, 0, 0, 9, 9, 0xFFFF);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, s_pixels[0]);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, s_pixels[9 * FB_W + 9]);
}

void test_rect_outline_only(void) {
    gfx_rect(&s_fb, 10, 10, 5, 5, 0xCAFE);
    /* Top edge */
    TEST_ASSERT_EQUAL_HEX16(0xCAFE, s_pixels[10 * FB_W + 10]);
    TEST_ASSERT_EQUAL_HEX16(0xCAFE, s_pixels[10 * FB_W + 14]);
    /* Bottom edge */
    TEST_ASSERT_EQUAL_HEX16(0xCAFE, s_pixels[14 * FB_W + 10]);
    /* Interior MUST be untouched */
    TEST_ASSERT_EQUAL_HEX16(0,      s_pixels[12 * FB_W + 12]);
}

void test_text_renders_some_pixels(void) {
    gfx_text_5x7(&s_fb, 0, 0, "A", 0xFFFF);
    int set = 0;
    for (int i = 0; i < FB_W * FB_H; ++i) if (s_pixels[i]) set++;
    /* Letter 'A' in a 5x7 cell sets at least 8 pixels. */
    TEST_ASSERT_TRUE_MESSAGE(set >= 8, "expected >=8 lit pixels for 'A'");
    /* Snapshot for human review. */
    ppm_write_rgb565("/tmp/test_text_A.ppm", s_pixels, FB_W, FB_H);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_clear_fills_all_pixels);
    RUN_TEST(test_pixel_writes_single_location);
    RUN_TEST(test_pixel_clips_oob);
    RUN_TEST(test_hline_writes_n_pixels);
    RUN_TEST(test_hline_clips_at_right_edge);
    RUN_TEST(test_vline_writes_n_pixels);
    RUN_TEST(test_line_diagonal_endpoints);
    RUN_TEST(test_rect_outline_only);
    RUN_TEST(test_text_renders_some_pixels);
    return UNITY_END();
}
