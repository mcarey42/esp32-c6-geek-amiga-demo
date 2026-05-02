# esp32demo — A Tour Through Time

An old-school demoscene production for the Waveshare ESP32-C6-Geek board.
Twelve scenes, six-to-eight minutes, looping. A linear show with no input.
The aesthetic walks through the demoscene's own history: strict 1990 megademo
era → 1992-3 design era → 1994-onward and a synthwave detour into today.

## 1. Goals & Non-Goals

### Goals

1. A continuous looping demo production (boots → plays → loops) that visually
   reads as authentic to the Amiga demoscene tradition.
2. Twelve named scenes spanning three eras with diegetic transitions where
   possible.
3. A small, clean "kernel + scenes" framework that makes adding scene 13
   a single-file change.
4. Hidden SD-card asset prefetch so streamed scenes feel snappy.
5. First implementation phase ships a vertical slice (3 representative scenes
   end-to-end) before the rest are built.

### Non-Goals (v1)

- Audio. No music, no SFX. Effects are timed by a free-running clock.
- Interactivity. No button input, no menu, no scene browser.
- Network features (Wi-Fi 6, BLE) — the C6 has them, the demo doesn't use them.
- Authentic chip-feature emulation (no copper-list-on-CPU framework, no
  blitter emulation). We just produce the *look*, by whatever means.
- Pixel-perfect 320×256 Amiga emulation. We render natively at 240×135 RGB565.

## 2. Target Hardware

- **Board:** Waveshare ESP32-C6-Geek
- **MCU:** ESP32-C6 (RISC-V single-core @ 160 MHz, 512 KB SRAM, ≥ 4 MB flash —
  exact size depends on module variant; verify at build, our binary fits in 4 MB)
- **Display:** ST7789 LCD, 240 × 135 px, 16-bit RGB565
- **Display bus:** SPI2_HOST @ 40 MHz, DMA-driven via `esp_lcd`
- **Storage:** microSD slot (TF card) — assumed SPI-mode based on board pinout
- **No onboard audio.** No DAC, no buzzer wired up.
- **LCD pin map** (from `crypto-tests/components/ui_lcd/include/lcd_pins.h`):
  MOSI=2, SCLK=1, CS=5, DC=3, RST=4, BL=6
- **SD pin map:** TBD during framework build (read from board schematic).
  Reserve SPI3_HOST for SD to avoid sharing SPI bus with the LCD.

### Performance budget (per frame at 30 fps target)

| Item | Budget |
|------|--------|
| Total frame budget | 33.3 ms |
| SPI blit (full 240×135 RGB565 @ 40 MHz, DMA) | ~13 ms (overlapped) |
| Compute available while DMA runs | ~33 ms |
| Per-pixel work (~32 K pixels) | ~1 µs/pixel max |

A scene that misses 30 fps drops to 15 fps for *itself only* (every other
render call). The director never starves; SD prefetch never blocks the
render task.

## 3. Architecture

A "kernel + scene overlays" model — the lineage the user identified from
their own demo-writing days.

```
                    ┌──────────────────────┐
                    │  director task       │
                    │  - timeline cursor   │
                    │  - scene swap        │
                    │  - transitions       │
                    └─────────┬────────────┘
                              │ tick(t, fb)
                              ▼
                    ┌──────────────────────┐
                    │  active scene        │
                    │  (one of 12 modules) │
                    │  init/render/free    │
                    └─────────┬────────────┘
                              │ writes pixels
                              ▼
       ┌──────────────────────────────────┐
       │  framebuffer manager             │
       │  - 2× RGB565 backbuffers (~64 KB │
       │    each, ping-pong in SRAM)      │
       │  - DMA flush via esp_lcd         │
       └─────────┬────────────────────────┘
                 │
                 ▼
            ST7789 LCD

  ┌──────────────────────┐         ┌──────────────────────┐
  │  asset prefetch task │ ◄─────  │  scene manifest      │
  │  (SD reads, LRU      │         │  (per scene: assets, │
  │   buffer pool)       │         │   est. duration)     │
  └──────────────────────┘         └──────────────────────┘
```

### 3.1 Tasks

| Task | Priority | Stack | Responsibility |
|------|---------:|------:|----------------|
| `director`   | 5 | 8 KB | Timeline, scene lifecycle, frame pacing, transitions |
| `prefetch`   | 3 | 6 KB | SD reads, asset buffer pool, decode/decompress |
| `lvgl`/idle  | — | —    | Not used in v1 (we drive the LCD directly) |

Single-core C6: tasks share the one core via FreeRTOS round-robin.
`prefetch` runs only during SD-bound idle and yields aggressively.

### 3.2 Scene API

Every scene is a C module that exposes the same struct:

```c
typedef struct scene_s {
    const char *name;                   // "copper_bars", "vector_parade", ...
    uint32_t    est_duration_ms;        // for the timeline budget
    const asset_manifest_t *assets;     // NULL if scene needs no SD assets

    esp_err_t (*init)   (void *ctx);                    // alloc working set
    void      (*render) (void *ctx, fb_t *fb,           // draw one frame
                         uint32_t scene_t_ms);
    void      (*teardown)(void *ctx);                   // free working set
    size_t    ctx_size;                                 // director allocates
} scene_t;
```

The director maintains a `static const scene_t *show[]` array — the timeline
in code form. Scenes never call each other; the director is the only thing
that switches them.

### 3.3 Framebuffer manager

- Two RGB565 buffers, 240 × 135 × 2 = **64,800 bytes each** (≈ 127 KB total).
- Allocated once at boot from internal SRAM. Never freed.
- `fb_t` struct gives scenes typed pixel access (`fb->pixels[y * 240 + x]`).
- Director calls `fb_present(fb)` after the scene's `render()` returns; that
  hands off to `esp_lcd_panel_draw_bitmap()` which DMAs over SPI.
- While DMA is running, the director hands the *other* buffer to the next
  `render()` call — overlap is automatic.

If 127 KB of SRAM proves too much (we have 512 KB but other things consume
it), v1.1 fallback: single buffer + tile-based rendering at 1/4 height. Spec
assumes ping-pong is fine; the build will validate.

### 3.4 Asset prefetch & SD streaming

- A scene's `asset_manifest_t` declares: `[{path, kind, target_buffer_size}]`.
- The director, on transition into scene N, signals `prefetch` to start
  loading scene N+1's assets into a pool buffer, while N is still rendering.
- For very large streams (e.g. 120 silhouette frames @ ~2 KB each = 240 KB)
  the manifest declares "stream" mode: only a small ring of N frames is
  resident at a time, and the prefetch task tops it up while the scene runs.
- SD is mounted as FAT via the IDF's `esp_vfs_fat_sdspi_mount()`.

### 3.5 Timeline & transitions

A static C array describes the show:

```c
static const timeline_entry_t timeline[] = {
    { .scene = &SCENE_BOOT_TITLE,       .duration_ms = 12000 },
    { .scene = &SCENE_COPPER_SCROLLER,  .duration_ms = 30000 },
    { .scene = &SCENE_STARFIELD,        .duration_ms = 18000 },
    { .scene = &SCENE_VECTOR_PARADE,    .duration_ms = 50000 },
    /* ... */
    { .transition = TRANSITION_HYPERSPACE, .duration_ms = 1500 },
    /* ... */
};
```

Transitions are first-class entries — small "mini-scenes" the director
runs between scenes. v1 ships **inter-scene** transitions:

- `TRANSITION_FADE_BLACK` — 0.5 s palette fade to black (universal default)
- `TRANSITION_PALETTE_WARP` — 1 s sweep that re-maps the current frame
- `TRANSITION_HYPERSPACE` — the diegetic jump from Scene 3 → Scene 4

Two effects in the storyboard ("crystallize" in Scene 6, "light burst" at
the end of Scene 10) are **intra-scene** — they happen inside the scene's
own `render()` function, with the next scene's first frame chosen to look
visually continuous. They are not director-level entries.

## 4. Scene Catalog

### Act I — The Megademo Era (1988-1991)

#### Scene 1 — Cold Boot & Title (~12 s)

2 s of fake-POST text on black ("CHECKING MEMORY... 65 K OK", a banner
line) then explodes into the title card: chunky logo with a copper-bar
wash behind, "PRESENTS — A TOUR THROUGH TIME" subtitle.

- **Class:** procedural
- **Assets:** none
- **New tech proven:** text rendering (5×7 bitmap font), copper-bar shader

#### Scene 2 — Copper Bars + Sine Scroller (~30 s)

Smooth horizontal palette bands fill the screen; a sine-wave-distorted
scroller crawls along the bottom with real demoscene "GREETZ" — Spaceballs,
Scoopex, Kefrens, Sanity, Anarchy, Future Crew, TBL. Bars do a subtle
"hit/shake" on scripted timeline beats.

- **Class:** procedural
- **Assets:** none (font baked in)
- **New tech proven:** sine-distorted blitting, palette cycling

#### Scene 3 — 3D Starfield + Hyperspace Jump (~18 s)

256 stars warp from infinity with parallax depth. ~12 s of calm starfield
with occasional camera tilts; final ~6 s ramps into a hyperspace jump
(star-streak rendering, white flash) that diegetically delivers Scene 4.

- **Class:** procedural
- **Assets:** none
- **New tech proven:** 3D point projection, motion-blur trick (line draw)
- **Transition out:** `TRANSITION_HYPERSPACE`

#### Scene 4 — Vector Parade (~50 s)

A sequence of seven rotating wireframe objects, each held ~5-7 s, morphing
where the topology allows:

1. **Cube** — the warmup
2. **Cylinder** — introduces hidden-line removal
3. **Octahedron → Icosahedron** — vertex morph between related solids
4. **Dodecahedron** — slow tumble, all faces
5. **X-Wing fighter** — silhouette + S-foil split animation
6. **TIE Fighter** — screams in; the two trade vector "shots" (line segments
   fired between them)
7. Both fly off; camera pulls back to reveal a Death-Star-style sphere on
   the horizon

- **Class:** procedural + small static models in flash
- **Assets:** model data baked into the binary (vertex+edge tables)
- **New tech proven:** rotation matrices, perspective projection, hidden-line
  removal, vertex morphing, multi-object scenes, simple animation rigs

### Act II — The Design Era (1992-1993)

#### Scene 5 — Plasma "Breathe" (~25 s)

Classic sin/cos plasma. Palette walks from angry red → calming blue/violet
across the scene. A single italic word ("breathe") fades in/out at the
emotional midpoint. Mood beat, not a flex beat.

- **Class:** procedural
- **Assets:** none
- **New tech proven:** plasma generator, palette LUT cycling

#### Scene 6 — Metaballs → Glenz Vectors (~25 s)

First half: translucent blobs merge, split, breed. Second half: the blobs
*crystallize* (per-pixel transition) into transparent shaded polyhedra
rotating in 3D — the Sanity look. One continuous transformation.

- **Class:** procedural
- **Assets:** none
- **New tech proven:** metaball field eval, polygon fill with alpha
- **Transition style:** intra-scene "crystallize" effect inside `render()`;
  the scene-level transition into Scene 7 is the standard fade

#### Scene 7 — Rotozoomer Texture Carousel (~20 s)

A 64×64 texture is rotated and scaled across the whole screen. Cycles
through three textures (checker → XOR pattern → mini logo), with per-pixel
noise dissolves between them.

- **Class:** procedural + tiny baked textures
- **Assets:** 3 × 64×64×2 B = 24 KB in flash
- **New tech proven:** affine warp, fixed-point UV step, dissolve transition

#### Scene 8 — Rotoscope Silhouette (SD) (~30 s)

The State of the Art homage. ~120 pre-traced silhouette frames stream from
SD. Single accent color over a moody background; accent color drifts
magenta → teal across the scene matching the act's palette walk.

- **Class:** SD streamed
- **Assets:** `silhouettes/frame_000.bin` … `frame_119.bin`
  (~2 KB each → ~240 KB total on SD; 4-frame ring resident)
- **New tech proven:** SD prefetch streaming, frame-pacing against I/O

### Act III — Beyond (1994-today)

#### Scene 9 — Voxel Landscape Sunset Flyover (~30 s)

Comanche-style heightmap. Camera flies over a desert; lighting/palette
walks from noon → deep sunset across the scene's 30 s.

- **Class:** procedural + heightmap on SD
- **Assets:** 1 × 256×256 heightmap + 1 × 256×256 colormap (~128 KB on SD,
  loaded once at scene init)
- **New tech proven:** voxel raycast, heightmap traversal, sky gradient

#### Scene 10 — Tunnel: Acceleration (~25 s)

Perspective tunnel with a scrolling textured wall. UV lookup table
pre-computed at scene init. Starts slow, builds speed; twist amount
oscillates. Final 3 s the textured walls dissolve to pure radial color
rings — we exit the tunnel into pure light.

- **Class:** procedural + LUT (computed at init, ~64 KB scratch)
- **Assets:** tiny tile texture baked into binary
- **New tech proven:** UV lookup table precompute, radial dissolve
- **Transition out:** intra-scene "light burst" ending; Scene 11 opens on
  a matching bright frame for visual continuity

#### Scene 11 — Synthwave Grid "2026" (~20 s)

Outrun-style perspective grid drifting forward, gradient sun on the horizon,
"2026" floating chrome-style above the sun. Smooth gradients show off
RGB565 vs the strict 16-color start of the demo.

- **Class:** procedural
- **Assets:** none (font procedurally chrome-shaded)
- **New tech proven:** perspective grid, gradient fills, chrome shading

#### Scene 12 — Credits + End Scroller (~25 s, then loop)

Long sine scroller crawling vertically with thanks/credits, fading copper
ribbon at the bottom, cameo of the wireframe X-Wing flying past behind the
scroller (callback to Scene 4). Fades to black, holds 2 s, restarts the
demo from Scene 1.

- **Class:** procedural
- **Assets:** none
- **New tech proven:** vertical sine scroller, scene cameo (re-using model
  from Scene 4)

## 5. File & Component Layout

```
esp32demo/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── main/
│   ├── CMakeLists.txt
│   ├── app_main.c                  # boot, mounts SD, starts director
│   └── timeline.c                  # the show: ordered scene list + transitions
├── components/
│   ├── lcd_drv/                    # ST7789 init + DMA blit (adapted from crypto-tests)
│   │   ├── include/lcd_pins.h
│   │   ├── lcd_drv.c
│   │   └── CMakeLists.txt
│   ├── fb/                         # framebuffer manager (ping-pong + present)
│   │   ├── include/fb.h
│   │   ├── fb.c
│   │   └── CMakeLists.txt
│   ├── director/                   # timeline cursor, scene lifecycle, transitions
│   │   ├── include/director.h
│   │   ├── include/scene.h         # the scene API struct
│   │   ├── director.c
│   │   ├── transitions.c
│   │   └── CMakeLists.txt
│   ├── prefetch/                   # SD asset prefetch task + buffer pool
│   │   ├── include/prefetch.h
│   │   ├── prefetch.c
│   │   └── CMakeLists.txt
│   ├── gfx/                        # shared 2D primitives (line, fill, blit, font)
│   │   ├── include/gfx.h
│   │   ├── gfx.c
│   │   ├── font_5x7.c
│   │   └── CMakeLists.txt
│   ├── gfx3d/                      # shared 3D math (mat4, vec3, project, edges)
│   │   ├── include/gfx3d.h
│   │   ├── gfx3d.c
│   │   └── CMakeLists.txt
│   └── scenes/                     # one .c per scene; one CMakeLists.txt for all
│       ├── CMakeLists.txt
│       ├── scene_01_boot_title.c
│       ├── scene_02_copper_scroller.c
│       ├── scene_03_starfield.c
│       ├── scene_04_vector_parade.c
│       ├── models/                 # vertex/edge tables for vector parade
│       │   ├── m_cube.c
│       │   ├── m_xwing.c
│       │   └── ...
│       ├── scene_05_plasma.c
│       ├── scene_06_metaballs_glenz.c
│       ├── scene_07_rotozoomer.c
│       ├── scene_08_silhouette.c
│       ├── scene_09_voxel.c
│       ├── scene_10_tunnel.c
│       ├── scene_11_synthwave.c
│       └── scene_12_credits.c
└── sdcard/                         # files copied to physical SD before running
    ├── silhouettes/
    │   ├── frame_000.bin
    │   └── ...
    └── voxel/
        ├── height.raw
        └── color.raw
```

Reusing the LCD driver pattern from `../crypto-tests/components/ui_lcd/`
(same panel, same pins, same `esp_lcd` setup) but stripping LVGL — we draw
to the framebuffer directly.

## 6. Build Phases (vertical slice first)

### Phase 1 — Framework + Vertical Slice

Goal: prove the architecture end-to-end with three scenes that touch every
asset class.

1. `lcd_drv` adapted from crypto-tests, no LVGL.
2. `fb` ping-pong manager + present.
3. `gfx` minimal 2D (clear, line, hline, vline, blit, 5×7 font).
4. `director` with one-scene timeline support, no transitions yet.
5. **Scene 2** — Copper Bars + Sine Scroller (pure procedural).
6. `gfx3d` (mat4, vec3, project, edge draw).
7. **Scene 4 (cube only)** — proves the 3D pipeline.
8. `prefetch` + SD mount.
9. **Scene 8** — Rotoscope Silhouette (proves SD streaming).
10. Two-entry timeline → three-entry with `TRANSITION_FADE_BLACK` between.

Output: a looping mini-demo of three scenes, ~90 s.

### Phase 2 — Fill out the show

Add scenes 1, 3, 5, 6, 7, 9, 10, 11, 12 in roughly this order; each is one
file, one CMakeLists addition, one timeline entry. Add the diegetic
transitions (`HYPERSPACE`, `CRYSTALLIZE`, `LIGHT_BURST`) when their
adjacent scenes both exist.

### Phase 3 — Polish pass

Beat-timing pass across all scenes, palette consistency check, final
durations tuned, demo-on-a-stick packaging (single `idf.py flash` produces
the runnable artifact).

## 7. Risks & Open Questions

- **SD pinout on the Geek board.** Need to read the schematic to confirm
  the TF slot pins. Phase-1 step 8 should start with that lookup, not a
  guess.
- **127 KB of SRAM for ping-pong** may collide with stack growth or other
  IDF allocations. Fallback: single buffer + half-height tiling. Validate
  empirically in Phase 1 step 2 before committing.
- **Voxel scene cost.** A naive Comanche raycast at 240 columns × 60 depth
  steps is ~14 K rays/frame; at 30 fps that's 430 K ray steps/sec. Tight
  but feasible on a 160 MHz RISC-V. May need to drop voxel to 15 fps for
  itself; spec allows this.
- **Silhouette streaming throughput.** SPI SD on ESP-IDF gives ~1-2 MB/s
  in practice. At ~2 KB/frame and 30 fps that's only 60 KB/s — plenty of
  headroom. But the 4-frame ring needs validation under load.
- **No audio = no obvious beat.** All "beat-synced" effects are scripted to
  a fixed timeline. If the timing feels arbitrary in v1, post-v1 we can
  always add I2S DAC + a tracker.

## 8. Definition of Done (v1)

The board, when powered on:

1. Boots to the title card within 3 s of power-on.
2. Plays all 12 scenes in order, holding scene durations within ±10 % of spec.
3. Frame rate stays ≥ 24 fps in every scene (stretch: ≥ 30 fps in 10/12 scenes).
4. Loops cleanly back to Scene 1 with no perceptible pause or glitch.
5. Runs continuously for ≥ 1 hour without crash, hang, or visible memory
   leak (heap free not declining over time).
6. Code is structured such that a 13th scene can be added by writing one
   `.c` file and adding two lines (CMakeLists + timeline).
