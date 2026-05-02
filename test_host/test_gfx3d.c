#include "unity.h"
#include "gfx3d.h"
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

#define EPS 1e-5f

void test_identity_times_identity(void) {
    mat4_t i = mat4_identity();
    mat4_t r = mat4_mul(i, i);
    for (int k = 0; k < 16; ++k) {
        TEST_ASSERT_FLOAT_WITHIN(EPS, i.m[k], r.m[k]);
    }
}

void test_translation_moves_point(void) {
    mat4_t t = mat4_translation(1.0f, 2.0f, 3.0f);
    vec3_t v = mat4_transform_point(t, (vec3_t){0, 0, 0});
    TEST_ASSERT_FLOAT_WITHIN(EPS, 1.0f, v.x);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 2.0f, v.y);
    TEST_ASSERT_FLOAT_WITHIN(EPS, 3.0f, v.z);
}

void test_rotation_y_90_degrees_swaps_x_z(void) {
    mat4_t r = mat4_rotation_y((float)M_PI / 2.0f);
    vec3_t v = mat4_transform_point(r, (vec3_t){1, 0, 0});
    /* Rotating (1,0,0) by 90deg around Y -> (0,0,-1) */
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f,  v.x);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f,  v.y);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -1.0f, v.z);
}

void test_perspective_projects_centered_point_to_screen_center(void) {
    mat4_t p = mat4_perspective((float)M_PI / 3.0f, (float)FB_W / FB_H, 0.1f, 100.0f);
    int sx, sy;
    int ok = gfx3d_project((vec3_t){0, 0, -5}, p, &sx, &sy);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_INT_WITHIN(2, FB_W / 2, sx);
    TEST_ASSERT_INT_WITHIN(2, FB_H / 2, sy);
}

void test_perspective_rejects_behind_near(void) {
    mat4_t p = mat4_perspective((float)M_PI / 3.0f, (float)FB_W / FB_H, 0.1f, 100.0f);
    int sx, sy;
    int ok = gfx3d_project((vec3_t){0, 0, 1.0f}, p, &sx, &sy);
    TEST_ASSERT_FALSE(ok);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_identity_times_identity);
    RUN_TEST(test_translation_moves_point);
    RUN_TEST(test_rotation_y_90_degrees_swaps_x_z);
    RUN_TEST(test_perspective_projects_centered_point_to_screen_center);
    RUN_TEST(test_perspective_rejects_behind_near);
    return UNITY_END();
}
