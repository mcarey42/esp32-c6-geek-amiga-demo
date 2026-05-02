#include "unity.h"
#include "prefetch.h"

void setUp(void) {}
void tearDown(void) {}

void test_ring_state_init(void) {
    ring_state_t s;
    ring_state_init(&s, 120, 4);
    TEST_ASSERT_EQUAL_INT(120, s.total);
    TEST_ASSERT_EQUAL_INT(4, s.capacity);
    TEST_ASSERT_EQUAL_INT(0, s.next_load);
    TEST_ASSERT_EQUAL_INT(0, s.oldest_loaded);
}

void test_ring_evicts_oldest_when_full(void) {
    ring_state_t s;
    ring_state_init(&s, 120, 4);
    /* Pretend frames 0..3 are loaded; need frame 4. Evict 0. */
    s.oldest_loaded = 0;
    s.next_load = 4;
    int victim = ring_state_next_evict(&s, 4);
    TEST_ASSERT_EQUAL_INT(0, victim);
}

void test_ring_eviction_for_random_access_picks_furthest_from_request(void) {
    ring_state_t s;
    ring_state_init(&s, 120, 4);
    s.oldest_loaded = 10;
    s.next_load = 14;
    /* Caller jumps backward to frame 9: evict the *furthest* loaded
     * (frame 13) since frames close to the request are about to be needed. */
    int victim = ring_state_next_evict(&s, 9);
    TEST_ASSERT_EQUAL_INT(13, victim);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ring_state_init);
    RUN_TEST(test_ring_evicts_oldest_when_full);
    RUN_TEST(test_ring_eviction_for_random_access_picks_furthest_from_request);
    return UNITY_END();
}
