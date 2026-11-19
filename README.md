# UZDoom NX2

Unofficial port of **[UZDoom](https://github.com/UZDoom/UZDoom) 5.0.3** to the
**Nintendo Switch 2**, as a homebrew `.nro` started by **kaiHEN**. Rendering is
**Vulkan** through **OunceVK** (Mesa's NVK for Switch 2), on **libounce**
(libnx for Switch 2). One file, `uzdoom-nx2.nro`, with a built-in launcher
(profiles, IWAD and mod libraries, load order) in English, French and
Portuguese (English by default).

Plays everything UZDoom loads: **.wad**, **.pk3**, **.ipk3**, **.pk7**,
**.zip**, **.deh/.bex**. The free **Freedoom** IWADs are included, so it is
playable right after installation.

> Status: **0.1.4, early.** Tested by one person on one console. Freedoom,
> DOOM II and DOOM II + Brutal Doom play at about 60 fps; see
> [Benchmarks](#benchmarks) and [CHANGELOG.md](CHANGELOG.md).

The engine is the official UZDoom 5.0.3 release (tag `5.0.3`, commit
`7910df7102033fd7754a317f3254f24101135926` of
[UZDoom/UZDoom](https://github.com/UZDoom/UZDoom)) plus the Switch patches in
`patches/`. (UZDoom is the continuation of GZDoom; a few names in the code and
in mod compatibility notes still say GZDoom.)

## Safe for your console

kaiHEN lets homebrew write anywhere, the system memory (NAND) included, where
a bad write can brick the console. UZDoom NX2 never writes there:

- everything it writes (settings, saves, logs, screenshots) stays in
  `switch/uzdoom-nx2` on the SD card, and a guard refuses any write anywhere
  else, even elsewhere on the SD card;
- the code that could open the system memory is not even in the program
  (checked automatically at every build);
- if the guard cannot start, the program does not start either.

Details for developers: [docs/SECURITY.md](docs/SECURITY.md).

## Install

See [docs/INSTALL.txt](docs/INSTALL.txt). In short: copy `switch/` to the root
of the SD card, put extra IWADs in `switch/uzdoom-nx2/iwads/` and mods in
`switch/uzdoom-nx2/mods/`, start it from hbmenu (a game started with R held).

In the game, gyro aiming and rumble are in Options > Controller Options (both
off by default).

## Benchmarks

Measured on a Switch 2 (docked, 1920x1080 output) from the logs of the
console tests (`switch/uzdoom-nx2/logs/perf.log`). Average frame rates over
the whole session, loading screens left out.

| Test | Version | Result |
|---|---|---|
| Freedoom: Phase 2, MAP01 (vsync, 60 fps cap) | 0.1.3 | 55 fps |
| DOOM II + Brutal Doom v21 (vsync, 60 fps cap) | 0.1.3 | 57 fps |
| **BENCH 1**: Brutal Doom + NUTS (10,000 monsters) | 0.1.3 | ~15 fps before the fight, ~1 fps once the monsters wake up |
| **BENCH 2**: Brutal Doom + NUTS3 (34,000 objects) | 0.1.3 | 12.5 fps before the fight, below 1 fps once they wake up |
| **BENCH 3**: Frozen Time, 4K internal, MSAA 4x, all effects, xBRZ 4x | 0.1.3 | 27 fps |
| **BENCH 4**: BENCH 3 + Brutal Doom | 0.1.3 | 23 fps (ran out of memory when the level restarted after a death; fixed in 0.1.4) |

What limits them:

- **BENCH 1 and 2 are CPU tests.** With tens of thousands of scripted
  monsters, game logic alone takes 26 ms per frame and preparing the scene
  49 ms. A desktop PC also slows down a lot on NUTS with Brutal Doom.
- **BENCH 3 and 4 are GPU tests.** The game itself takes 3-7 ms per frame;
  the rest is the GPU drawing 3840x2160 with 4x multisampling and every
  post-process effect.

The benchmark profiles, their mods and where to download them are in
`tools/bench/` (the mods are not part of the release).

### Compatibility tests

Four more profiles in `tools/tests/` test the engine's features rather than
its speed:

| Profile | What it tests |
|---|---|
| TEST 1 - MyHouse.wad (Veddge, 2023) | UDMF maps, ACS scripting, portals, mirrors, camera textures, map changes in scripts |
| TEST 2 - Hideous Destructor 5.0.1a | one of the heaviest ZScript mods (the script VM runs without a JIT on the console) |
| TEST 3 - The Adventures of Square | a standalone game as IWAD: 3D floors, slopes, portals, ACS menus and cutscenes |
| TEST 4 - Batman Doom (1999) | an old total conversion with a DeHackEd patch |

When something goes wrong, the logs are in `switch/uzdoom-nx2/logs/`
(`crash.log`, `error.log`, `engine-session.txt`, `rdoom.log`, `engine.log`,
`perf.log`).

## Build

```sh
git clone https://github.com/ouncebrew/ounce-devkit
docker build -t ounce-devkit ounce-devkit
git clone https://github.com/UZDoom/UZDoom engine
git -C engine checkout -b uzdoom-nx2 7910df7102033fd7754a317f3254f24101135926
git -C engine am ../patches/*.patch
scripts/build.sh
```

Produces `dist/uzdoom-nx2.nro` and `dist/UZDoom-NX2-<version>.zip` (with
Freedoom).

## Origin and changes from the Switch 1 port

Based on the engine patches and launcher of **RDoom NX** by Renan (Æon)
([aeon1911/RDoom-NX](https://github.com/aeon1911/RDoom-NX), GPLv3), a Vulkan
port of UZDoom to the Switch 1. Changes for the Switch 2:

- libounce and OunceVK instead of libnx and a Mesa SDK; Cortex-A78C CPU;
- VI2 display window (no binder queue): `VK_NN_vi_surface` at 1920×1080,
  60 fps at 60 and 120 Hz, VRR allowed;
- `vkGet*ProcAddr` trampolines for the extension commands OunceVK does not
  export (debug utils, surface capabilities 2, ray tracing);
- Switch 1 / Ryujinx workarounds removed (TPIDR_EL0 bridge, nvhost-as
  compatibility, binder and Mesa GL wraps);
- writes limited to `switch/uzdoom-nx2` on the SD card;
- official UZDoom logo and colours; launcher and loading screen in English,
  French and Portuguese.

## Licences

- Code: **GPLv3** (UZDoom's licence; RDoom NX's changes are GPLv3 too). See
  `engine/LICENSE` and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
- UZDoom logo: © 2025 The UZDoom Team, designed by Carlos "Cardboard Marty"
  Sanchez, **CC BY-SA 4.0**. The icon and launcher logos are made from it
  (`assets/src/make_art.py`) and are under the same licence.
- Freedoom: BSD-style licence (`third_party/freedoom/COPYING.txt`).

"Doom" is a trademark of id Software. This project is not affiliated with
id Software, Bethesda, Nintendo or the UZDoom team.
