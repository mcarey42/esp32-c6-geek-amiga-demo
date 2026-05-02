#!/usr/bin/env python3
"""Generate 60 placeholder silhouette frames as 1-bit packed bitmaps.
Output: sdcard/silhouettes/frame_NNN.bin, 60x100, 750 bytes each.
The 'silhouette' is just a bouncing oval — proves the pipeline works
end-to-end. Real rotoscoped frames replace these in Phase 3."""

import math
import os

W, H, FRAMES = 60, 100, 60
OUT_DIR = "sdcard/silhouettes"
os.makedirs(OUT_DIR, exist_ok=True)

for f in range(FRAMES):
    bits = bytearray((W * H + 7) // 8)
    cy = int(50 + 30 * math.sin(2 * math.pi * f / FRAMES))
    cx = 30
    for y in range(H):
        for x in range(W):
            inside = ((x - cx) / 18.0) ** 2 + ((y - cy) / 28.0) ** 2 <= 1.0
            if inside:
                byte = (y * W + x) // 8
                bit = (y * W + x) % 8
                bits[byte] |= (1 << bit)
    path = os.path.join(OUT_DIR, f"frame_{f:03d}.bin")
    with open(path, "wb") as out:
        out.write(bits)
print(f"wrote {FRAMES} frames to {OUT_DIR}")
