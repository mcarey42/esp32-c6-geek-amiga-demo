#include "scene.h"
#include "fb.h"
#include <stdint.h>

/* Apply a uniform black darken to the existing fb based on progress 0..1.
 * 0 = no change, 1 = fully black. */
static void darken(fb_t *fb, float k)
{
    if (k <= 0) return;
    uint8_t scale = (uint8_t)((1.0f - k) * 32);  /* clamp to 5-bit channels */
    for (int i = 0; i < fb->w * fb->h; ++i) {
        uint16_t p = fb->pixels[i];
        uint8_t r = (p >> 11) & 0x1F;
        uint8_t g = (p >> 5)  & 0x3F;
        uint8_t b = (p)       & 0x1F;
        r = (uint8_t)(r * scale / 32);
        g = (uint8_t)(g * (scale * 2) / 64);
        b = (uint8_t)(b * scale / 32);
        fb->pixels[i] = (r << 11) | (g << 5) | b;
    }
}

void transition_fade_black_apply(fb_t *fb, uint32_t t_in_transition_ms,
                                 uint32_t total_ms)
{
    if (total_ms == 0) return;
    float k = (float)t_in_transition_ms / (float)total_ms;
    if (k > 1.0f) k = 1.0f;
    darken(fb, k);
}
