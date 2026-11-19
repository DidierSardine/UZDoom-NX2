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

> Status: **0.1.1, early.** The first console test (0.1.0) found a black screen
> with big mods and a GPU loss in levels; 0.1.1 fixes their causes (see
> [CHANGELOG.md](CHANGELOG.md)) but has not been re-tested yet.

The engine is the official UZDoom 5.0.3 release (tag `5.0.3`, commit
`7910df7102033fd7754a317f3254f24101135926` of
[UZDoom/UZDoom](https://github.com/UZDoom/UZDoom)) plus the Switch patches in
`patches/`. (UZDoom is the continuation of GZDoom; a few names in the code and
in mod compatibility notes still say GZDoom.)

## Safety: zero NAND writes

kaiHEN does not restrict homebrew: a program can write to the system NAND and
brick the console. UZDoom NX2 protects it with three independent layers
(details: [docs/SECURITY.md](docs/SECURITY.md)):

1. **Nothing in the binary can open the NAND.** Every build checks the final
   ELF (statically linked, dead code removed) and rejects it if it contains any
   function that opens a BIS partition, save data or a system storage, or that
   changes system settings, installed content, tickets, Bluetooth pairing,
   album, clocks or hardware (about 80 patterns), or kaiHEN's API. If the
   check fails, the `.nro` is deleted.
2. **SD card write firewall** (`nx2_fsguard.c`). Installed in `userAppInit`,
   before any C++ constructor and before any file is opened: newlib's `sdmc`
   device entry is replaced with a copy whose write operations (open for
   writing/create/truncate, delete, rename, mkdir, rmdir, chmod, utimes,
   links) fail with `EACCES` outside `sdmc:/switch/uzdoom-nx2`. This covers
   all code: engine, launcher, ZMusic, SDL, the Vulkan driver. Every other
   file system device is unmounted. If the firewall cannot be installed, the
   program refuses to start.
3. **Strict path policy** (`nx2_pathpolicy.c`): canonical paths, `..`, device
   prefixes, ambiguous names (`...`, trailing dots/spaces), refused
   characters. 54 unit tests on the PC.

The program does not need to read the NAND either, so the NAND is never
opened at all.

## Install

See [docs/INSTALL.txt](docs/INSTALL.txt). In short: copy `switch/` to the root
of the SD card, put extra IWADs in `switch/uzdoom-nx2/iwads/` and mods in
`switch/uzdoom-nx2/mods/`, start it from hbmenu (a game started with R held).

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

## What still has to be checked on the console

- the switch from the launcher to the game (Vulkan presenting on the Switch 2's
  VI2 display layer);
- sound (SDL2's audio renderer on Switch 2);
- controllers (Joy-Con 2, Pro Controller).

Logs to send when something goes wrong: `switch/uzdoom-nx2/logs/` (`crash.log`,
`engine-session.txt`, `rdoom.log`, `engine.log`, `engine-stderr.log`).

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
- SD firewall and NAND checks; everything under `switch/uzdoom-nx2`;
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
