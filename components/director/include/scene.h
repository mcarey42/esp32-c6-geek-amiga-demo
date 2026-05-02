#pragma once
#include <stdint.h>
#include <stddef.h>
#include "fb.h"

typedef struct asset_manifest_s asset_manifest_t;  /* defined later by prefetch */

typedef struct scene_s {
    const char *name;
    uint32_t    est_duration_ms;
    const asset_manifest_t *assets;          /* NULL if none */

    int   (*init)    (void *ctx);
    void  (*render)  (void *ctx, fb_t *fb, uint32_t scene_t_ms);
    void  (*teardown)(void *ctx);
    size_t ctx_size;
} scene_t;

typedef struct timeline_entry_s {
    const scene_t *scene;       /* NULL means "transition" — Task 9 */
    int            transition;  /* unused until Task 9 */
    uint32_t       duration_ms;
} timeline_entry_t;
