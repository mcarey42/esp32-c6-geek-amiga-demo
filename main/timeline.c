#include "scene.h"
#include "scenes.h"

const timeline_entry_t TIMELINE[] = {
    { .scene = &SCENE_02_COPPER_SCROLLER, .duration_ms = 30000 },
    { .transition = TRANSITION_FADE_BLACK, .duration_ms = 500 },
    { .scene = &SCENE_04_CUBE,             .duration_ms = 8000 },
    { .transition = TRANSITION_FADE_BLACK, .duration_ms = 500 },
};

const int TIMELINE_LEN = sizeof(TIMELINE) / sizeof(TIMELINE[0]);
