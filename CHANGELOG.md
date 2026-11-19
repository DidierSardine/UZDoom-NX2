# Changelog

## 0.1.2 (2026-11-19)

From the second console test (0.1.1: DOOM II + Reelism Gold fine).

- **Console:** A opens the Switch keyboard and its OK now runs the command
  (the converted Enter key was dropped); B closes the console; X erases;
  the D-Pad walks the history.
- **Rumble works:** devkitPro's SDL2 never set up the vibration of the first
  controller and sent out-of-range values. UZDoom's haptics now drive the
  HID vibration of the active controller (handheld, Pro Controller, Joy-Con).
- **Gyro aiming:** on by default; "Gyro aiming" and "Gyro sensitivity" in the
  profile editor; the left stick click (`gyro_reset`) levels the view.
  Console variables: `nx_gyro`, `nx_gyro_sensitivity`, `nx_gyro_invert_x`,
  `nx_gyro_invert_y`, `nx_gyro_deadzone`.
- **Freedoom: Phase 1 froze on its first 3D frame:** ZCULL (new in OunceVK
  20261118) is off again for the scene depth, as in 0.1.0 where that game
  ran. A GPU that stops now ends the game with an error after 20 s instead
  of freezing it, and `hang.log` records where the game thread was.
- OunceVK's `mkdir -p` of its cache folder no longer shows as refused writes.
- Benchmark profiles (on the test SD card, not in the package): Brutal Doom
  v21 + NUTS / NUTS3, Frozen Time.

## 0.1.1 (2026-11-19)

Fixes from the first test on a Switch 2 (logs in `evidence/hw/2026-11-19-first-test/`,
not published).

- **Black screen with big mods (DOOM II + Reelism Gold).** Vulkan failed to
  start ("out of device memory": OunceVK could not allocate its first 32 MB
  segment although 8.3 GB of heap were free). RDoom's object pool, a
  workaround for newlib's slow malloc on the Switch 1, took its 128 KB-aligned
  slabs from libounce's small heap-page pool, which mimalloc also needs for
  large blocks. The pool is off on the Switch 2 (malloc is mimalloc there).
- **Errors were invisible.** On the Switch 2 one producer presents to the
  display layer at a time; the error console could not show while Vulkan
  still held it. Vulkan is now released before the error screen.
- **GPU lost a few seconds into a level ("completion join failed").** The
  driver found its command memory overwritten. Changes:
  - OunceVK 20261118 (GPU memory registration and NAK fixes) instead of
    20261117.1;
  - RDoom's Switch 1 (Tegra X1) renderer workarounds default to UZDoom's own
    behaviour on the Switch 2: two frames in flight, early submits, triangle
    fans as indexed triangles, half-resolution effects layer, background
    sprite/texture decoding and uploads, linear-image uploads, dynamic
    resolution, effect stabilizer, emulated depth bias, transient depth
    buffers, RGBA8 scene. Configs saved by 0.1.0 are migrated once
    (`rd_defaults_rev` 4).
- **Mod library: A toggles.** A on a mod already in the profile removes it
  (the file stays on the SD card).
- OunceVK's shader cache goes to `switch/uzdoom-nx2/cache/ouncevk`
  (`MESA_SHADER_CACHE_DIR`) instead of `sdmc:/config/ouncevk`, which the SD
  firewall refused; it is cleared with the other shader caches.
- perf.log reports the real heap use (libounce commits the whole heap to
  mimalloc, so the process always looked full).

## 0.1.0 (2026-11-18)

First release: UZDoom 5.0.3 on Switch 2 (kaiHEN, libounce, OunceVK), SD
write firewall and NAND checks, launcher in English/French/Portuguese,
official UZDoom branding, Freedoom included.
