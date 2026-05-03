# Installing on the ESP32-C6-Geek

This guide walks an end-user from "I have a Waveshare ESP32-C6-Geek
board in my hand" to "the demo is playing on the LCD."

If you're comfortable with ESP-IDF already, skip to [Quick install](#quick-install).
Otherwise read [Detailed setup](#detailed-setup) for IDF installation help.

## What you need

- A **Waveshare ESP32-C6-Geek** board ([product page][waveshare-product])
- A **USB-C cable** (data, not just power)
- A computer running Linux, macOS, or Windows (WSL2 recommended for Windows)
- ~3 GB of disk space for ESP-IDF
- ~30 minutes for the first install (mostly IDF install); subsequent
  rebuilds take seconds

[waveshare-product]: https://www.waveshare.com/esp32-c6-geek.htm

## Quick install

If you have ESP-IDF v5.3+ already sourced in your shell:

```bash
git clone https://github.com/mcarey42/esp32-c6-geek-amiga-demo.git
cd esp32-c6-geek-amiga-demo
./build-and-flash.sh
```

The script will: build the firmware → auto-detect the serial port the
board enumerated as → flash it → open the serial monitor so you can
watch the boot log. The demo starts as soon as the board boots —
look at the LCD.

To exit the monitor, press **Ctrl+]**.

## Detailed setup

### 1. Install ESP-IDF v6.0 (or v5.3+)

The project was developed against **ESP-IDF v6.0** but should build on
v5.3 and later.

#### Linux / macOS (recommended path)

Espressif's installer scripts handle everything (toolchain, Python venv,
udev rules):

```bash
mkdir -p ~/esp && cd ~/esp
git clone -b release/v6.0 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32c6
. ./export.sh
```

The `. ./export.sh` part has to happen in **every shell** where you
want to use IDF. Many people add a function to their `~/.bashrc` or
`~/.zshrc`:

```bash
get_idf() { . ~/esp/esp-idf/export.sh; }
```

#### Other platforms

See Espressif's official guide:
https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/get-started/

### 2. Get the demo source

```bash
git clone https://github.com/mcarey42/esp32-c6-geek-amiga-demo.git
cd esp32-c6-geek-amiga-demo
```

If you want a specific milestone, check out a tag:

```bash
git checkout phase-3.5-complete   # all features
git checkout phase-2-complete     # 12 scenes, no Star Wars / glenz
git checkout phase-1-vertical-slice  # framework + 3 representative scenes
```

### 3. Plug in the board

Use a **data-capable USB-C cable** (some cables are charge-only). The
board will enumerate as a serial device:

- **Linux:** typically `/dev/ttyACM0`. If you get permission errors,
  add yourself to the `dialout` group: `sudo usermod -a -G dialout $USER`,
  then log out + back in.
- **macOS:** typically `/dev/cu.usbmodem*`.
- **Windows / WSL2:** see Espressif's docs; you may need `usbipd` to
  forward the USB device to WSL.

### 4. Build and flash

```bash
./build-and-flash.sh
```

This script:

1. Sources IDF (if not already in env, looks in common install paths)
2. Sets target to esp32c6
3. Builds
4. Auto-detects the serial port
5. Flashes
6. Opens the serial monitor

### Manual build/flash (if you don't want the script)

```bash
. ~/esp/esp-idf/export.sh        # or wherever your IDF lives
idf.py set-target esp32c6
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

## What you should see

Within ~150 ms of the board booting:

1. **LCD lights up** with a green `ESP-DEMOSYSTEM v1.0` banner and the
   POST lines printing one by one with a blinking cursor.
2. After ~2 seconds, a brief flash, then the chunky `ESP32DEMO` title
   card with `PRESENTS - A TOUR THROUGH TIME` over a copper bar wash.
3. The 12-scene loop runs continuously (~270 seconds total per loop):

| # | Scene | ~Duration |
|---|---|---|
| 01 | Boot/Title | 12s |
| 02 | Copper bars + sine scroller | 30s |
| 03 | 3D starfield → hyperspace | 18s |
| 04 | Vector parade (cube → ... → Death Star) | 45s |
| 05 | Plasma "breathe" | 25s |
| 06 | Metaballs → glenz crystal | 25s |
| 07 | Rotozoomer | 20s |
| 08 | Silhouette | 30s |
| 09 | Voxel landscape | 30s |
| 10 | Tunnel → light | 25s |
| 11 | Synthwave grid | 20s |
| 12 | Credits | 25s |

In the serial monitor you'll see the director logging each scene
transition, e.g.:

```
I (12765) director: -> scene copper_scroller (30000 ms)
I (38274) director: -> scene starfield (18000 ms)
I (56292) director: -> scene vector_parade (45000 ms)
```

## Troubleshooting

### `idf.py: command not found`
You haven't sourced IDF in your current shell. Run
`. ~/esp/esp-idf/export.sh` (or wherever IDF lives) and retry.

### `Permission denied: /dev/ttyACM0` (Linux)
Add yourself to the `dialout` group (`sudo usermod -a -G dialout $USER`),
log out and back in. Or use `sudo` for one-off testing.

### `A fatal error occurred: Failed to connect to ESP32-C6` during flash
Press the BOOT button on the board *while* you start `idf.py flash`,
or hold BOOT + tap RESET to force download mode. Some USB hubs are
flaky — try a direct connection if possible.

### Screen is blank / shows garbage
The most common cause is flashing a build for the wrong target. Run
`idf.py set-target esp32c6` and rebuild. If you have multiple boards
plugged in, double-check the serial port matches the right one.

### "no serial device found" from the script
Pass the port explicitly: `./build-and-flash.sh --port /dev/ttyACM0`
or whatever your OS reports.

### Demo runs but I want to see the boot log without flashing again
```bash
. ~/esp/esp-idf/export.sh
idf.py -p /dev/ttyACM0 monitor
# Reset the board (or unplug + replug). Ctrl+] to exit monitor.
```

## Customizing

- **Scene order, durations, transitions:** edit `main/timeline.c`.
- **Add a scene:** drop a new `scene_NN_xxx.c` in `components/scenes/`,
  add it to that component's `CMakeLists.txt`, declare its symbol in
  `components/scenes/include/scenes.h`, and add an entry to `main/timeline.c`.
- **Scenes use only public APIs from `components/{fb,gfx,gfx3d}`** —
  there's no shared mutable state between them, so they compose cleanly.
- See `docs/superpowers/specs/2026-05-02-amiga-demo-design.md` for the
  full design discussion.

## Running host tests

Pure-C math, primitives, director cursor logic, and per-scene snapshots
all run on Linux/macOS via Unity:

```bash
./test_host/run.sh
```

Snapshot PPMs from each scene land in `/tmp/scene_*.ppm` for human
inspection (open with `feh`, `display`, or any Netpbm-aware viewer).

## Hardware reference

The Waveshare ESP32-C6-Geek board:

- **MCU:** ESP32-C6 single-core RISC-V @ 160 MHz
- **RAM:** 512 KB SRAM
- **Flash:** 16 MB (the demo uses ~280 KB; partition table reserves 8 MB
  for the app)
- **LCD:** ST7789, 240×135 pixels, RGB565
- **LCD pins (per Waveshare schematic):** MOSI=2, SCLK=1, CS=5, DC=3,
  RST=4, BL=6 — connected via SPI2_HOST at 40 MHz with DMA
- **microSD slot:** present but **not used** (Phase 1 dropped the SD path
  after discovering the C6 has only one general-purpose SPI host;
  see `docs/superpowers/specs/` for the architectural backstory)

## License

MIT — see [LICENSE](LICENSE). Embedded font is BSD-2-Clause from
Adafruit.
