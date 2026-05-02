#pragma once
#include "scene.h"

/* Pure-logic timeline cursor — extracted from the FreeRTOS task so it
 * can be tested on the host. */
typedef struct {
    const timeline_entry_t *entries;
    int                     n_entries;
    int                     idx;
    uint32_t                t_in_scene_ms;
} cursor_t;

void cursor_init   (cursor_t *c, const timeline_entry_t *entries, int n);
void cursor_advance(cursor_t *c, uint32_t dt_ms);
const timeline_entry_t *cursor_current(const cursor_t *c);

/* Boots the director task. Returns once the task is running. */
int  director_start(const timeline_entry_t *entries, int n);
