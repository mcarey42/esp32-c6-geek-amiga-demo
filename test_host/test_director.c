#include "unity.h"
#include "director.h"
#include "scene.h"

static const scene_t SCENE_A = { .name = "A", .est_duration_ms = 0 };
static const scene_t SCENE_B = { .name = "B", .est_duration_ms = 0 };

void setUp(void) {}
void tearDown(void) {}

void test_cursor_starts_at_first_entry(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 1000 },
        { .scene = &SCENE_B, .duration_ms = 2000 },
    };
    cursor_t c;
    cursor_init(&c, e, 2);
    TEST_ASSERT_EQUAL_PTR(&SCENE_A, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(0, c.t_in_scene_ms);
}

void test_cursor_advances_within_scene(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 1000 },
        { .scene = &SCENE_B, .duration_ms = 2000 },
    };
    cursor_t c;
    cursor_init(&c, e, 2);
    cursor_advance(&c, 500);
    TEST_ASSERT_EQUAL_PTR(&SCENE_A, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(500, c.t_in_scene_ms);
}

void test_cursor_swaps_scene_on_overflow(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 1000 },
        { .scene = &SCENE_B, .duration_ms = 2000 },
    };
    cursor_t c;
    cursor_init(&c, e, 2);
    cursor_advance(&c, 1200);  /* crosses A->B */
    TEST_ASSERT_EQUAL_PTR(&SCENE_B, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(200, c.t_in_scene_ms);
}

void test_cursor_loops_at_end(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 1000 },
        { .scene = &SCENE_B, .duration_ms = 2000 },
    };
    cursor_t c;
    cursor_init(&c, e, 2);
    cursor_advance(&c, 3500);  /* past end of B (1000+2000), wraps */
    TEST_ASSERT_EQUAL_PTR(&SCENE_A, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(500, c.t_in_scene_ms);
}

void test_cursor_advance_zero_dt_unchanged(void) {
    timeline_entry_t e[] = { { .scene = &SCENE_A, .duration_ms = 1000 } };
    cursor_t c;
    cursor_init(&c, e, 1);
    cursor_advance(&c, 0);
    TEST_ASSERT_EQUAL_PTR(&SCENE_A, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(0, c.t_in_scene_ms);
}

void test_cursor_advance_exact_boundary_swaps(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 100 },
        { .scene = &SCENE_B, .duration_ms = 100 },
    };
    cursor_t c;
    cursor_init(&c, e, 2);
    cursor_advance(&c, 100);
    TEST_ASSERT_EQUAL_PTR(&SCENE_B, cursor_current(&c)->scene);
    TEST_ASSERT_EQUAL_UINT32(0, c.t_in_scene_ms);
}

void test_cursor_advance_multi_scene_in_one_step(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 100 },
        { .scene = &SCENE_B, .duration_ms = 100 },
        { .scene = &SCENE_A, .duration_ms = 100 },
    };
    cursor_t c;
    cursor_init(&c, e, 3);
    cursor_advance(&c, 250);
    TEST_ASSERT_EQUAL_INT(2, c.idx);
    TEST_ASSERT_EQUAL_UINT32(50, c.t_in_scene_ms);
}

void test_cursor_advance_past_full_ring_loops(void) {
    timeline_entry_t e[] = {
        { .scene = &SCENE_A, .duration_ms = 100 },
        { .scene = &SCENE_B, .duration_ms = 100 },
        { .scene = &SCENE_A, .duration_ms = 100 },
    };
    cursor_t c;
    cursor_init(&c, e, 3);
    cursor_advance(&c, 950);  /* 3 * 100 + 650 = wraps 3x then 50ms into idx 0 */
    TEST_ASSERT_EQUAL_INT(0, c.idx);
    TEST_ASSERT_EQUAL_UINT32(50, c.t_in_scene_ms);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cursor_starts_at_first_entry);
    RUN_TEST(test_cursor_advances_within_scene);
    RUN_TEST(test_cursor_swaps_scene_on_overflow);
    RUN_TEST(test_cursor_loops_at_end);
    RUN_TEST(test_cursor_advance_zero_dt_unchanged);
    RUN_TEST(test_cursor_advance_exact_boundary_swaps);
    RUN_TEST(test_cursor_advance_multi_scene_in_one_step);
    RUN_TEST(test_cursor_advance_past_full_ring_loops);
    return UNITY_END();
}
