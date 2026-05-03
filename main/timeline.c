#include "scene.h"
#include "scenes.h"

/* Phase 1 + Phase 2 (so far) — six scenes, fade between each.
 * Order follows the spec's "tour through time" arc:
 *   Act I  (megademo era):  01 boot/title -> 02 copper -> 03 starfield -> 04 cube
 *   Act II (design era):    05 plasma "breathe" -> 08 silhouette
 *   Act III (beyond):       (more in upcoming Phase 2 work)
 *
 * Total loop ~125s with the current set. */
const timeline_entry_t TIMELINE[] = {
    { .scene = &SCENE_01_BOOT_TITLE,       .duration_ms = 12000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_02_COPPER_SCROLLER,  .duration_ms = 25000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_03_STARFIELD,        .duration_ms = 18000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_04_CUBE,             .duration_ms = 8000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_05_PLASMA,           .duration_ms = 25000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_08_SILHOUETTE,       .duration_ms = 30000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
};

const int TIMELINE_LEN = sizeof(TIMELINE) / sizeof(TIMELINE[0]);
