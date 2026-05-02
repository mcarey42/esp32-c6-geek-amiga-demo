#include <stdio.h>
#include <stdint.h>

/* Write a P6 PPM file from an RGB565 buffer. Used by tests to snapshot
 * rendered output for human inspection. Returns 0 on success. */
int ppm_write_rgb565(const char *path, const uint16_t *fb, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; ++i) {
        uint16_t p = fb[i];
        uint8_t r = (uint8_t)(((p >> 11) & 0x1F) * 255 / 31);
        uint8_t g = (uint8_t)(((p >> 5)  & 0x3F) * 255 / 63);
        uint8_t b = (uint8_t)((p & 0x1F)  * 255 / 31);
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
    return 0;
}

/* Header-style declaration used by tests via extern. */
