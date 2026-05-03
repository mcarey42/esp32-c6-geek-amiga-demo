#!/usr/bin/env python3
"""Generate 60 silhouette frames as 1-bit packed bitmaps.

Outputs:
  1. sdcard/silhouettes/frame_NNN.bin (60x100, 750 bytes each) -- kept
     as a useful artefact for future user-supplied frame workflows.
  2. components/scenes/scene_08_frames.c -- a const C array baked into
     flash rodata. This is what the ESP32 firmware actually reads at
     runtime in Phase 1+ (SD path was dropped: see commit dropping SD).

Phase 3 upgrade: stylized swaying humanoid silhouette (head, torso,
arms, legs with counter-balanced motion). One full sway cycle per
60 frames -- at the silhouette scene's 20 fps, that's a 3-second loop
which feels meditative and matches the moody background. A real Phase 4
would replace this generator with traced video frames of an actual
dancer; this is the procedural stand-in.
"""

import math
import os

W, H, FRAMES = 60, 100, 60
FRAME_BYTES = (W * H + 7) // 8   # 750
OUT_DIR = "sdcard/silhouettes"
C_OUT = "components/scenes/scene_08_frames.c"

os.makedirs(OUT_DIR, exist_ok=True)
os.makedirs(os.path.dirname(C_OUT), exist_ok=True)


def fill_disc(bits, cx, cy, r):
    r2 = r * r
    y0 = max(0, int(cy - r))
    y1 = min(H, int(cy + r) + 1)
    x0 = max(0, int(cx - r))
    x1 = min(W, int(cx + r) + 1)
    for y in range(y0, y1):
        dy = y - cy
        for x in range(x0, x1):
            dx = x - cx
            if dx * dx + dy * dy <= r2:
                idx = y * W + x
                bits[idx >> 3] |= 1 << (idx & 7)


def fill_thick_segment(bits, x0, y0, x1, y1, half_w):
    dx = x1 - x0
    dy = y1 - y0
    length = max(1.0, math.sqrt(dx * dx + dy * dy))
    steps = int(length) + 1
    for i in range(steps + 1):
        t = i / steps
        cx = x0 + dx * t
        cy = y0 + dy * t
        fill_disc(bits, cx, cy, half_w)


def humanoid_frame(f):
    """One frame of the swaying humanoid. f in 0..FRAMES-1."""
    bits = bytearray(FRAME_BYTES)
    phase = 2 * math.pi * f / FRAMES
    sway = math.sin(phase) * 4.0

    body_cx = W / 2 + sway
    body_cy = 56

    # --- Head ---
    head_cx = body_cx + math.sin(phase) * 1.0
    head_cy = body_cy - 24
    head_r = 7
    fill_disc(bits, head_cx, head_cy, head_r)
    fill_thick_segment(bits, head_cx, head_cy + head_r - 1,
                       body_cx, body_cy - 12, 3)

    # --- Torso (oval-ish) ---
    torso_top_y = body_cy - 12
    torso_bot_y = body_cy + 14
    fill_thick_segment(bits, body_cx, torso_top_y, body_cx, torso_bot_y, 7)
    fill_thick_segment(bits, body_cx - 8, torso_top_y + 1,
                       body_cx + 8, torso_top_y + 1, 4)

    # --- Arms (counter-swing to body sway) ---
    shoulder_l = (body_cx - 8, torso_top_y + 1)
    shoulder_r = (body_cx + 8, torso_top_y + 1)
    arm_swing = math.sin(phase + math.pi)

    elbow_l = (shoulder_l[0] - 6 + arm_swing * 4,
               shoulder_l[1] + 9 - arm_swing * 2)
    hand_l  = (elbow_l[0] - 2 + arm_swing * 6,
               elbow_l[1] + 9 - arm_swing * 5)
    fill_thick_segment(bits, shoulder_l[0], shoulder_l[1],
                       elbow_l[0], elbow_l[1], 3)
    fill_thick_segment(bits, elbow_l[0], elbow_l[1], hand_l[0], hand_l[1], 2)
    fill_disc(bits, hand_l[0], hand_l[1], 2)

    elbow_r = (shoulder_r[0] + 6 - arm_swing * 4,
               shoulder_r[1] + 9 + arm_swing * 2)
    hand_r  = (elbow_r[0] + 2 - arm_swing * 6,
               elbow_r[1] + 9 + arm_swing * 5)
    fill_thick_segment(bits, shoulder_r[0], shoulder_r[1],
                       elbow_r[0], elbow_r[1], 3)
    fill_thick_segment(bits, elbow_r[0], elbow_r[1], hand_r[0], hand_r[1], 2)
    fill_disc(bits, hand_r[0], hand_r[1], 2)

    # --- Legs (swing with body sway) ---
    hip_l = (body_cx - 4, torso_bot_y)
    hip_r = (body_cx + 4, torso_bot_y)
    leg_swing = math.sin(phase)

    knee_l = (hip_l[0] - 2 - leg_swing * 3, hip_l[1] + 11)
    foot_l = (knee_l[0] - 1 - leg_swing * 4, knee_l[1] + 12)
    fill_thick_segment(bits, hip_l[0], hip_l[1], knee_l[0], knee_l[1], 3)
    fill_thick_segment(bits, knee_l[0], knee_l[1], foot_l[0], foot_l[1], 3)

    knee_r = (hip_r[0] + 2 + leg_swing * 3, hip_r[1] + 11)
    foot_r = (knee_r[0] + 1 + leg_swing * 4, knee_r[1] + 12)
    fill_thick_segment(bits, hip_r[0], hip_r[1], knee_r[0], knee_r[1], 3)
    fill_thick_segment(bits, knee_r[0], knee_r[1], foot_r[0], foot_r[1], 3)

    return bits


frames = []
for f in range(FRAMES):
    bits = humanoid_frame(f)
    path = os.path.join(OUT_DIR, f"frame_{f:03d}.bin")
    with open(path, "wb") as out:
        out.write(bits)
    frames.append(bytes(bits))
print(f"wrote {FRAMES} frames to {OUT_DIR}")


def fmt_frame(idx, data):
    lines = [f"    {{ /* frame {idx} */"]
    for chunk_start in range(0, len(data), 16):
        chunk = data[chunk_start:chunk_start + 16]
        hex_bytes = ", ".join(f"0x{b:02x}" for b in chunk)
        suffix = "," if chunk_start + 16 < len(data) else ""
        lines.append(f"        {hex_bytes}{suffix}")
    lines.append("    }")
    return "\n".join(lines)


with open(C_OUT, "w") as out:
    out.write("/* Auto-generated by tools/gen_silhouette_placeholder.py -- DO NOT EDIT.\n")
    out.write(f" * {FRAMES} frames x {FRAME_BYTES} bytes ({W}x{H} 1-bit packed).\n")
    out.write(" * Phase 3 upgrade: stylized swaying humanoid silhouette.\n")
    out.write(" * Lives in flash rodata; consumed by components/scenes/scene_08_silhouette.c.\n")
    out.write(" */\n")
    out.write("#include <stdint.h>\n\n")
    out.write(f"const uint8_t SCENE_08_FRAMES[{FRAMES}][{FRAME_BYTES}] = {{\n")
    out.write(",\n".join(fmt_frame(i, frames[i]) for i in range(FRAMES)))
    out.write("\n};\n\n")
    out.write(f"const int SCENE_08_FRAME_COUNT = {FRAMES};\n")
    out.write(f"const int SCENE_08_FRAME_BYTES = {FRAME_BYTES};\n")

print(f"wrote {C_OUT} ({os.path.getsize(C_OUT)} bytes)")
