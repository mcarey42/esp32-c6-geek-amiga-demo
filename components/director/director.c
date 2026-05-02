#include "director.h"
#include "fb.h"

#ifdef ESP_PLATFORM
#  include "freertos/FreeRTOS.h"
#  include "freertos/task.h"
#  include "esp_timer.h"
#  include "esp_log.h"
#  include <stdlib.h>
static const char *TAG = "director";
#else
#  include <stdlib.h>
#endif

#ifndef ESP_PLATFORM
#include <assert.h>
#endif

void cursor_init(cursor_t *c, const timeline_entry_t *e, int n)
{
#ifdef ESP_PLATFORM
    if (n <= 0) { ESP_LOGE(TAG, "cursor_init: n=%d invalid", n); abort(); }
    for (int i = 0; i < n; ++i) {
        if (e[i].duration_ms == 0) {
            ESP_LOGE(TAG, "cursor_init: entry %d has duration_ms=0", i);
            abort();
        }
    }
#else
    assert(n > 0);
    for (int i = 0; i < n; ++i) assert(e[i].duration_ms > 0);
#endif
    c->entries = e;
    c->n_entries = n;
    c->idx = 0;
    c->t_in_scene_ms = 0;
}

void cursor_advance(cursor_t *c, uint32_t dt_ms)
{
    c->t_in_scene_ms += dt_ms;
    while (c->t_in_scene_ms >= c->entries[c->idx].duration_ms) {
        c->t_in_scene_ms -= c->entries[c->idx].duration_ms;
        c->idx = (c->idx + 1) % c->n_entries;
    }
}

const timeline_entry_t *cursor_current(const cursor_t *c)
{
    return &c->entries[c->idx];
}

#ifdef ESP_PLATFORM

#define FRAME_INTERVAL_MS 33  /* ~30 fps target */

typedef struct {
    cursor_t cursor;
    const scene_t *active;
    void *ctx;
} director_state_t;

static void enter_scene(director_state_t *s, const scene_t *sc)
{
    if (s->active) {
        if (s->active->teardown) s->active->teardown(s->ctx);
        free(s->ctx);
        s->ctx = NULL;
    }
    s->active = sc;
    if (!sc->render) {
        ESP_LOGE(TAG, "scene %s missing render", sc->name);
        abort();
    }
    if (sc->ctx_size) {
        s->ctx = calloc(1, sc->ctx_size);
        if (!s->ctx) { ESP_LOGE(TAG, "ctx alloc failed for %s", sc->name); abort(); }
    }
    if (sc->init && sc->init(s->ctx) != 0) {
        ESP_LOGE(TAG, "scene %s init failed", sc->name);
        abort();
    }
    ESP_LOGI(TAG, "-> scene %s (%u ms)", sc->name, (unsigned)sc->est_duration_ms);
}

static void director_task(void *arg)
{
    director_state_t *s = arg;
    enter_scene(s, cursor_current(&s->cursor)->scene);

    TickType_t last_wake = xTaskGetTickCount();
    int64_t prev = esp_timer_get_time();
    for (;;) {
        int64_t now = esp_timer_get_time();
        uint32_t dt = (uint32_t)((now - prev) / 1000);
        prev = now;
        /* Cap dt: a long stall (SD blocking, etc) shouldn't fast-forward
         * through scenes the user never saw. */
        if (dt > 2 * FRAME_INTERVAL_MS) dt = 2 * FRAME_INTERVAL_MS;

        const scene_t *want = cursor_current(&s->cursor)->scene;
        if (want != s->active) enter_scene(s, want);

        fb_t *fb = fb_acquire_back();
        s->active->render(s->ctx, fb, s->cursor.t_in_scene_ms);
        (void)fb_present();

        cursor_advance(&s->cursor, dt);

        /* Fixed-rate pacing: vTaskDelayUntil absorbs render-time jitter so
         * the average frame interval stays at FRAME_INTERVAL_MS. */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(FRAME_INTERVAL_MS));
    }
}

int director_start(const timeline_entry_t *entries, int n)
{
    static director_state_t s_state;
    cursor_init(&s_state.cursor, entries, n);
    if (xTaskCreate(director_task, "director", 8192, &s_state, 5, NULL) != pdPASS)
        return -1;
    return 0;
}

#endif  /* ESP_PLATFORM */
