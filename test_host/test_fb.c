#include "unity.h"
#include "fb.h"

void setUp(void) {}
void tearDown(void) {}

void test_rgb565_pure_red(void) {
    TEST_ASSERT_EQUAL_HEX16(0xF800, fb_rgb565(255, 0, 0));
}

void test_rgb565_pure_green(void) {
    TEST_ASSERT_EQUAL_HEX16(0x07E0, fb_rgb565(0, 255, 0));
}

void test_rgb565_pure_blue(void) {
    TEST_ASSERT_EQUAL_HEX16(0x001F, fb_rgb565(0, 0, 255));
}

void test_rgb565_white(void) {
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, fb_rgb565(255, 255, 255));
}

void test_index_origin(void) {
    TEST_ASSERT_EQUAL_size_t(0, fb_index(0, 0));
}

void test_index_row_step(void) {
    TEST_ASSERT_EQUAL_size_t(FB_W, fb_index(0, 1));
}

void test_index_arbitrary(void) {
    TEST_ASSERT_EQUAL_size_t(10 + 5 * FB_W, fb_index(10, 5));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rgb565_pure_red);
    RUN_TEST(test_rgb565_pure_green);
    RUN_TEST(test_rgb565_pure_blue);
    RUN_TEST(test_rgb565_white);
    RUN_TEST(test_index_origin);
    RUN_TEST(test_index_row_step);
    RUN_TEST(test_index_arbitrary);
    return UNITY_END();
}
