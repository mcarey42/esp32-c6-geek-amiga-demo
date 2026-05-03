#include "scenes.h"
#include "fb.h"
#include "gfx.h"
#include <string.h>

#define SIL_W      60
#define SIL_H      100
#define SIL_FRAMES 60
#define SIL_FPS    20
#define SIL_FRAME_BYTES ((SIL_W * SIL_H + 7) / 8)

/* Set by app_main at boot via scene_08_silhouette_set_frames() before the
 * director starts. NULL means "no SD data loaded" — render falls back to
 * a placeholder so the demo doesn't crash. */
static const uint8_t *s_frames;

void scene_08_silhouette_set_frames(const void *frames)
{
    s_frames = frames;
}

static void render_bits(fb_t *fb, const uint8_t *bits, uint16_t color)
{
    int x0 = (FB_W - SIL_W) / 2;
    int y0 = (FB_H - SIL_H) / 2;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    for (int y = 0; y < SIL_H; ++y) {
        for (int x = 0; x < SIL_W; ++x) {
            int bit_idx = y * SIL_W + x;
            if (bits[bit_idx >> 3] & (1 << (bit_idx & 7))) {
                int fx = x0 + x, fy = y0 + y;
                if (fx >= 0 && fx < FB_W && fy >= 0 && fy < FB_H)
                    fb->pixels[fy * FB_W + fx] = color;
            }
        }
    }
}

/* Exposed for host tests — same render but with caller-supplied buffer. */
void scene_08_render_with_buffer(fb_t *fb, const void *frame_bits,
                                 int w, int h, uint16_t color)
{
    if (!frame_bits) return;
    int x0 = (fb->w - w) / 2;
    int y0 = (fb->h - h) / 2;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    const uint8_t *p = frame_bits;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int bit_idx = y * w + x;
            if (p[bit_idx >> 3] & (1 << (bit_idx & 7))) {
                int fx = x0 + x, fy = y0 + y;
                if (fx >= 0 && fx < fb->w && fy >= 0 && fy < fb->h)
                    fb->pixels[fy * fb->w + fx] = color;
            }
        }
    }
}

static void render(void *ctx, fb_t *fb, uint32_t t)
{
    (void)ctx;
    /* Moody dark-purple background. */
    for (int i = 0; i < fb->w * fb->h; ++i) fb->pixels[i] = fb_rgb565(20, 0, 40);

    if (!s_frames) {
        /* No SD data loaded — draw a visible placeholder so the user knows
         * the scene is running and SD failed to mount. */
        const char *msg = "NO SD DATA";
        int text_x = (FB_W - 10 * 6) / 2;     /* 10 chars * 6 px = 60 */
        int text_y = FB_H / 2 - 12;
        gfx_text_5x7(fb, text_x, text_y, msg, fb_rgb565(255, 200, 0));

        const char *msg2 = "(silhouette scene)";
        int t2x = (FB_W - 18 * 6) / 2;
        int t2y = FB_H / 2 + 4;
        gfx_text_5x7(fb, t2x, t2y, msg2, fb_rgb565(140, 60, 110));

        /* Pulsing dot row to prove the scene is live. */
        int dots = 12;
        int spacing = 8;
        int row_x = (FB_W - dots * spacing) / 2;
        int row_y = FB_H / 2 + 24;
        int active = (int)((t / 100) % dots);
        for (int i = 0; i < dots; ++i) {
            int dx = row_x + i * spacing + 4;
            uint16_t c = (i == active) ? fb_rgb565(255, 200, 0)
                                       : fb_rgb565(60, 30, 70);
            gfx_pixel(fb, dx, row_y, c);
            gfx_pixel(fb, dx + 1, row_y, c);
            gfx_pixel(fb, dx, row_y + 1, c);
            gfx_pixel(fb, dx + 1, row_y + 1, c);
        }
        return;
    }

    int frame = (int)((t * SIL_FPS / 1000) % SIL_FRAMES);
    const uint8_t *bits = s_frames + (size_t)frame * SIL_FRAME_BYTES;
    render_bits(fb, bits, fb_rgb565(255, 80, 200));
}

const scene_t SCENE_08_SILHOUETTE = {
    .name = "silhouette",
    .est_duration_ms = 30000,
    .render = render,
};
