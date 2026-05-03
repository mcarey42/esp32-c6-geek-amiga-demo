#include "scene.h"
#include "scenes.h"

/* Final 12-scene "tour through time" timeline. Total loop ~245s.
 *
 *   Act I  (megademo era):   01 boot/title -> 02 copper -> 03 starfield -> 04 cube
 *   Act II (design era):     05 plasma -> 06 metaballs -> 07 rotozoomer -> 08 silhouette
 *   Act III (beyond):        09 voxel -> 10 tunnel -> 11 synthwave -> 12 credits */
const timeline_entry_t TIMELINE[] = {
    { .scene = &SCENE_01_BOOT_TITLE,       .duration_ms = 12000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_02_COPPER_SCROLLER,  .duration_ms = 25000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_03_STARFIELD,        .duration_ms = 18000 },
    /* HYPERSPACE: scene 03 ends in white flash, scene 04 fades in from
     * white. No FADE_BLACK between them — the white delivers us. */
    { .scene = &SCENE_04_CUBE,             .duration_ms = 45000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_05_PLASMA,           .duration_ms = 25000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_06_METABALLS,        .duration_ms = 25000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_07_ROTOZOOMER,       .duration_ms = 20000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_08_SILHOUETTE,       .duration_ms = 20000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_09_VOXEL,            .duration_ms = 25000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_10_TUNNEL,           .duration_ms = 25000 },
    /* LIGHT_BURST: scene 10 exits into light, scene 11 fades in from
     * white. No FADE_BLACK — the white delivers us into the synthwave. */
    { .scene = &SCENE_11_SYNTHWAVE,        .duration_ms = 20000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
    { .scene = &SCENE_12_CREDITS,          .duration_ms = 25000 },
    { .transition = TRANSITION_FADE_BLACK,                        .duration_ms = 500 },
};

const int TIMELINE_LEN = sizeof(TIMELINE) / sizeof(TIMELINE[0]);
