# AGENTS.md — UZDoom NX2 (read before changing the project)

Port of **UZDoom 5.0.3** (official tag `5.0.3`, commit `7910df71` of
UZDoom/UZDoom) to the **Nintendo Switch 2**, as a homebrew `.nro` started by
**kaiHEN** (unrestricted CFW), rendered with **Vulkan** (OunceVK = Mesa's NVK)
on **libounce** (libnx for Switch 2). One executable, `uzdoom-nx2.nro`, holding
the launcher (profiles, IWADs, mods) and the engine.

Base: the RDoom NX engine fork (aeon1911/RDoom-NX-engine, branch `rdoom-nx`,
UZDoom 5.0.3 + 66 Switch 1 commits, GPLv3) and its launcher. Our commits are on
branch `uzdoom-nx2` of `engine/` (nested git repository, ignored by the parent
repository); the whole series is exported in `patches/`.

## Rule 1: ZERO NAND writes, everything in sdmc:/switch/uzdoom-nx2

kaiHEN gives homebrew full FS access (NAND included). Three layers, never to be
weakened (details in `docs/SECURITY.md`):

1. **Symbols** — `tests/check_nand_safety.sh` rejects the binary if the final
   ELF contains a libnx/libounce/libkaihen API that opens the NAND, save data
   or system storages, or changes settings/content/tickets/pairing/clocks/
   hardware. It also checks that the firewall is linked and called from
   `userAppInit`, and that the port's sources use no raw IPC and no FS API
   below the C library.
2. **devoptab firewall** — `engine/src/common/platform/posix/nx/nx2_fsguard.c`
   replaces newlib's `sdmc` entry in `userAppInit` (before any constructor).
   Every create/write/truncate/delete/rename/mkdir/rmdir/chmod/utimes outside
   `sdmc:/switch/uzdoom-nx2` fails (EACCES). Other FS devices are unmounted.
   If installation fails, the program shows an error and does not start.
3. **Path policy** — `nx2_pathpolicy.c` (pure C): canonical paths, `..`,
   device prefixes, ambiguous names and characters refused. Tested on the PC
   by `tests/pathpolicy_tests.c` (`scripts/test.sh`).

Forbidden: `fsOpenBis*`, `fsOpen_*`, any `fsdevMount*` but the SD card,
`fsdevGetDeviceFileSystem`, `setsysSet*`, `ns*`, `ncm*`, `kaihen*`, raw IPC
(`serviceDispatch`, `smGetService`) in the port's code, absolute paths outside
the sandbox. `/switch` itself is never created. NAND reads are not needed and
the NAND is never opened.

## Other rules

- Everything in English: code, comments, docs, commit messages. The launcher
  and the port's screens are translated (English default, French, Portuguese):
  every user-visible launcher string goes through `L()` and gets a line in
  `launcher/src/core/i18n_strings.inc`; engine-side screens use `NX2_Tr()`.
- Vulkan only (OunceVK); a Vulkan failure is fatal with a message.
- No overclocking (`tests/check_no_overclock.sh`). Tic rate stays 35 Hz.
- Official UZDoom branding only (`engine/branding`, CC BY-SA 4.0, attribution
  in the package).
- Never ship commercial IWADs or mods. Freedoom (BSD) is shipped.
- Console tests are the user's: never write "tested on Switch 2" without
  their logs.

## Build

```
docker build -t ounce-devkit <checkout of ouncebrew/ounce-devkit>
scripts/build.sh          # image + host tools + .nro + NAND checks + package + tests
scripts/dev-rebuild.sh    # incremental + NAND checks + package
scripts/test.sh           # host tests (path policy, launcher, languages, package)
```

## Switch 2 code map

| File | Role |
|---|---|
| `engine/src/common/platform/posix/nx/nx2_fsguard.c` | SD firewall (layer 2) |
| `engine/src/common/platform/posix/nx/nx2_pathpolicy.c` | path policy (layer 3) |
| `engine/src/common/platform/posix/nx/nx2_lang.cpp` | language of the port's screens |
| `engine/src/common/platform/posix/nx/nx2_motion.cpp` | rumble and gyro aiming (HID) |
| `engine/src/common/platform/posix/nx/nx2_parallel.cpp` | `parallel_for` on the process's CPU cores (xBRZ) |
| `engine/src/common/platform/posix/nx/i_nxsystem.cpp` | entry, 16 MB stack, crash.log, fatal error without firewall |
| `engine/src/common/platform/posix/nx/rdoom_nx.cpp` | SD layout, logs, launcher platform, writer thread |
| `engine/src/common/platform/posix/sdl/sdlglvideo.cpp` | `VK_NN_vi_surface` 1920×1080 on the VI2 window |
| `engine/libraries/ZVulkan/src/volk/volk.c` | no loader on Horizon: commands linked statically, extension trampolines |
| `launcher/` | launcher (profiles, IWAD/mod libraries, i18n) |
| `assets/src/make_art.py` | icon and launcher art from the official UZDoom branding |
| `tests/check_nand_safety.sh` | layer 1 |
