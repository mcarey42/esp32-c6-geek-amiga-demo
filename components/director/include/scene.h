#pragma once
#include <stdint.h>
#include <stddef.h>
#include "fb.h"

typedef struct asset_manifest_s asset_manifest_t;  /* defined later by prefetch */

/* Contract:
 *   - render MUST be non-NULL.
 *   - init   MAY be NULL (no per-scene init needed).
 *   - teardown MAY be NULL.
 *   - assets MAY be NULL.
 *   - ctx_size MAY be 0 (no ctx allocated).
 *
 *   - For boot-time-loaded assets (e.g. SD-streamed bitmaps that can't be
 *     re-loaded mid-scene because of bus contention), the scene exposes a
 *     scene_NN_xxx_set_assets(const void *) symbol. app_main loads the
 *     data and calls the setter BEFORE director_start(). The buffer must
 *     outlive the demo run. Scene 8 (silhouette) is the canonical example.
 */
typedef struct scene_s {
    const char *name;
    uint32_t    est_duration_ms;
    const asset_manifest_t *assets;          /* NULL if none */

    int   (*init)    (void *ctx);
    void  (*render)  (void *ctx, fb_t *fb, uint32_t scene_t_ms);
    void  (*teardown)(void *ctx);
    size_t ctx_size;
} scene_t;

typedef enum {
    TRANSITION_NONE = 0,
    TRANSITION_FADE_BLACK,
    /* TRANSITION_PALETTE_WARP, TRANSITION_HYPERSPACE -- added in Phase 2 */
} transition_kind_t;

typedef struct timeline_entry_s {
    const scene_t    *scene;        /* NULL means "transition" -- Task 9 */
    transition_kind_t transition;
    uint32_t          duration_ms;
} timeline_entry_t;
