# Changelog

## 0.1.4 (2026-11-19)

From the fourth console test (BENCH 2, 3 and 4; logs and a hang.log).

- **BENCH 4 ended on a black screen after the player's death.** Restarting
  the level ran out of memory ("Could not allocate memory for vulkan buffer",
  8.1 of 8.9 GB in use). Three causes, all fixed:
  - RDoom's texture eviction and its texture warm-up budget measured memory
    with `mallinfo()`, which reports nothing under the Switch 2's mimalloc:
    neither ever kicked in. They now read the heap's own counters, and
    pictures not drawn for a while lose their GPU copy once 5.5 GB are in use
    (`rd_vk_texevict_mb`, config migrated once: `rd_defaults_rev` 6);
  - the textures a restarted level no longer needs are destroyed before the
    new ones are created (Vulkan only queued them until the next frame);
  - the warm-up counted upscaled (xBRZ) pictures at their original size.
  BENCH 4 now uses xBRZ 2x: Brutal Doom's thousands of sprite frames at 4x
  do not fit in memory.
- **Long pause when the level restarts after a death** (about 6 s in BENCH 3):
  the xBRZ upscaling of every texture ran on one CPU core. It now uses all six
  cores the system gives the game (`parallel_for` on the Switch 2).
- The error screen's message is saved to `logs/error.log` as it says (it went
  to the folder above), and `rdoom.log` records whether the graphics were
  released before it.
- `rdoom.log` records the process's CPU cores.
- README: benchmark results, plainer safety notes. Four compatibility test
  profiles (`tools/tests/`): MyHouse.wad, Hideous Destructor, The Adventures
  of Square, Batman Doom.

## 0.1.3 (2026-11-19)

From the third console test (logs and a hang.log).

- **Freezes on the first frame of a level** (Freedoom: Phase 1, Brutal Doom
  + DOOM II, BENCH 2 and 3, stuck on the loading screen): hang.log showed
  the game thread waiting in `RenderBSP` for the `gl_multithread` worker,
  which never ran. That worker is off by default on the Switch 2.
- **Gyro aiming in the game's Controller Options**, off by default:
  horizontal and vertical sensitivity (3.5 / 1.5: faster sideways, slower
  up and down), inversion, "Level the view now". The reset button can be
  rebound in Customize Controls > Actions (left stick click by default).
  Smoother: the rotation is integrated sample by sample. The launcher no
  longer has gyro settings.
- **Rumble off by default** (haptics strength 0; Controller Options to turn
  it on). A frozen game no longer keeps the controller vibrating.
- **Default profiles:** only Freedoom 2 and Custom (the others needed mods
  that are not shipped).
- **Benchmarks:** BENCH 3 and 4 are real GPU tests now (3840x2160 internal,
  MSAA 4x, bloom, tonemap, lens flares, shadow maps, 16x anisotropic, xBRZ
  4x textures).

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
