# esp32-c6-geek-amiga-demo

An old-school demoscene production for the Waveshare **ESP32-C6-Geek** board —
a "tour through time" that walks the demoscene's own history across twelve
scenes, from the strict 1990 megademo era through the 1992-3 design era and
on into a modern synthwave detour.

Twelve scenes, three diegetic transitions, ~270 second loop, runs entirely
from internal flash on the C6's single 160 MHz RISC-V core. No SD card
required. No GPU, no FPU, no copper, no blitter — every pixel is computed
on the CPU and DMA'd out over SPI to the 240×135 ST7789 LCD.

A tribute to Spaceballs, Scoopex, Kefrens, Sanity, Anarchy, Future Crew,
TBL, and everyone else who made the 7 MHz Amiga 500 sing.

## The scenes

| # | Scene | Era |
|---|---|---|
| 01 | Cold boot + chunky title card | megademo |
| 02 | Copper bars + sine scroller (greetz) | megademo |
| 03 | 3D starfield → hyperspace jump → white flash | megademo |
| 04 | **Vector parade** — cube → cylinder → octahedron → icosahedron → dodecahedron → X-Wing (S-foils unlock) → X-Wing/TIE dogfight → pullback + Death Star reveal | megademo |
| 05 | Plasma "breathe" — palette walk red→violet, word fades in/out | design |
| 06 | Metaballs **crystallize** into glenz octahedron (alpha-blended faces) | design |
| 07 | Rotozoomer — 3 textures (checker, XOR, rings) cycle and warp | design |
| 08 | Silhouette — stylized swaying humanoid (placeholder rotoscope) | design |
| 09 | Voxel landscape — Comanche flyover, palette walks noon→sunset | beyond |
| 10 | Tunnel — perspective + twist, ramps to **exit-into-light** | beyond |
| 11 | Synthwave grid + outrun sun + chrome "2026" | beyond |
| 12 | Credits + greetz + copper ribbon + X-Wing cameo | beyond |

The HYPERSPACE (3→4), CRYSTALLIZE (within 6), and LIGHT_BURST (10→11)
transitions are diegetic — *"this thing became that thing"* rather than
fade-to-black between them.

## Hardware

- **Board:** Waveshare ESP32-C6-Geek
- **MCU:** ESP32-C6 RISC-V single-core @ 160 MHz, 512 KB SRAM, 16 MB flash
- **Display:** ST7789 LCD, 240×135 px, RGB565, SPI @ 40 MHz with DMA
- **Storage:** Not used. The 45 KB silhouette set is baked into flash;
  everything else is procedural.

## Architecture

A "kernel + scene plugins" model. The `director` task drives a fixed-rate
timeline (≈30 fps) and lifecycle-manages each scene. A `fb` ping-pong
framebuffer manager owns two RGB565 buffers in DMA-capable internal SRAM;
one is rendered into while the other is blitted. Pure-C math and primitives
are TDD'd on the host with Unity (21 host assertions) and visualized via
PPM snapshots. Hardware verification was done on real boards.

```
+--------------------+      +-------------------+
|  director task     |----->|  active scene     |
|  - timeline cursor |      |  init/render/free |
|  - scene swap      |      |  per-frame tick   |
|  - transitions     |      +---------+---------+
+--------------------+                |
                                      v writes pixels
                +---------------------+----------------+
                |  framebuffer manager                 |
                |  - 2x RGB565 backbuffers (~64KB ea)  |
                |  - DMA flush via esp_lcd             |
                +-------------------+------------------+
                                    |
                                    v
                              ST7789 LCD
```

Adding a 13th scene is one new `.c` file in `components/scenes/`, one line
in that component's `CMakeLists.txt`, and one entry in `main/timeline.c`.

## Building

Requires **ESP-IDF v6.0** (or compatible). The `scripts/activate.sh` shim
loads IDF v6.0 from `~/.espressif/v6.0/esp-idf` if you installed via
Espressif's eim tool — adjust to wherever your IDF lives, or just source
your IDF's own `export.sh` before running `idf.py`.

```bash
. scripts/activate.sh             # or: . ~/esp/v6.0/esp-idf/export.sh
idf.py set-target esp32c6
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Plug in the board over USB-C, hit reset, watch the show.

## Testing

Pure-C math, primitives, director cursor logic, and per-scene snapshot
sanity checks all run on the host (Linux/macOS) via Unity:

```bash
./test_host/run.sh
# 18 tests, ~30 unity assertions, runs in <1s
```

Snapshot PPMs land in `/tmp/scene_*.ppm` for human inspection — open with
`feh /tmp/scene_*.ppm` or any image viewer that handles Netpbm.

## Project history

This whole project — spec, plan, framework, twelve scenes, three diegetic
transitions, hardware verification — was developed collaboratively between
a human (the author) and Claude over a single conversational session. The
design doc (`docs/superpowers/specs/2026-05-02-amiga-demo-design.md`) and
the implementation plan (`docs/superpowers/plans/2026-05-02-amiga-demo-phase1.md`)
are committed alongside the code as a record of how it came together.

Tags worth checking out:

- `phase-1-vertical-slice` — framework + 3 representative scenes
- `phase-2-complete` — all 12 scenes wired end-to-end
- `phase-3-complete` — vector parade + diegetic transitions + humanoid silhouette
- `phase-3.5-complete` — CRYSTALLIZE + alpha-blend triangle primitive

## What's not yet here

- **Audio.** The C6-Geek has no DAC. Adding an I2S DAC breakout +
  tracker would unlock beat-synced effects.
- **Real rotoscoped silhouettes** in Scene 08. The current humanoid is
  procedural; tracing a video to bitmap frames would land closer to the
  State of the Art mood.
- **A 1-hour soak verification** for stability sign-off.

## Greetz

Forever to the demoscene legends who proved that hardware constraints
breed art, not limit it.

## License

Code: MIT (or whatever you prefer — this repo doesn't yet declare one).
The Adafruit GFX-derived 5×7 font in `components/gfx/font_5x7.c` is
BSD-2-Clause; attribution is in that file's header.
