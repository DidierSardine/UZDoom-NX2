# NAND safety and writes — UZDoom NX2 audit

Requirements: **no write that could damage the NAND**, and **every write inside
`sdmc:/switch/uzdoom-nx2`**. Reading the NAND would be acceptable, but the
program has no need for it, so it never opens the NAND at all (a NAND file
system, once open, could also be written).

## 1. Threat model

kaiHEN gives every homebrew a full FS session (a clone of the `fsp-srv` root
held by HEN): a program can open the BIS partitions (SYSTEM, USER, SAFE,
PRODINFO…), system save data, `set:sys` settings, and so on. A bad write there
can make the console unusable. The SD card cannot brick the console, but the
requirement is to write only in our own folder.

Ways a program could write:

| Way | How UZDoom NX2 closes it |
|---|---|
| C library calls (fopen, open, mkdir, rename, remove…) to the SD card | devoptab firewall (`nx2_fsguard.c`) + policy (`nx2_pathpolicy.c`) |
| Another devoptab device (save:, bis:, user:…) | none is mounted; the firewall unmounts any unknown file system device at start-up; the policy refuses every prefix but `sdmc:` |
| Direct libnx FS API (`fsOpenBis*`, `fsOpen_*`, `fsdevMountSaveData`…) | not in the binary (symbol check, the build is rejected otherwise) |
| `fsdevGetDeviceFileSystem` / direct `fsFs*` (bypassing the C library) | not in the binary / forbidden in the port's sources |
| System services that change the NAND (`set:sys`, `ns`, `ncm`, `es`, `btm`, album, `nifm`…) | not in the binary (symbol check) |
| kaiHEN API (FS root, loader, plugins) | not in the binary |
| Raw IPC (`serviceDispatch`, `smGetService`) in the port's code | forbidden by the source check |
| Mesa's on-disk shader cache | disabled on Horizon in OunceVK; RDoom's clean-up of `sdmc:/.mesa` was removed |

## 2. Layer 1 — binary check (`tests/check_nand_safety.sh`)

Run by `scripts/build.sh` and `scripts/dev-rebuild.sh` on the final, unstripped
ELF (static link, `--gc-sections`: only reachable code is in it). The `.nro`
is **deleted** when the check fails, and `scripts/package.sh` refuses to
package without a passed check. It verifies:

- no symbol matches one of ~80 forbidden patterns (FS: NAND, save data,
  storages, non-SD mounts; system settings, content, tickets, pairing, album,
  network, parental controls, clocks, hardware buses, secure monitor,
  debugging other processes, kaiHEN);
- the firewall is linked (`NX2_FsGuardInstall`, `NX2_PathCheck`, `G_open`,
  `G_unlink`, `G_rename`, `G_mkdir`, `G_rmdir`, `G_chdir`) and `userAppInit`
  calls `NX2_FsGuardInstall` (checked in the disassembly);
- the port's sources (`engine/.../nx/`, `launcher/src/`) use neither raw IPC
  nor the FS API below the C library, nor an absolute SD path outside the
  sandbox (except a line marked "nx2: read-only path").

Manual review of the 0.1.0 ELF: the only FS write functions in it are
`fsFsCreateFile/DeleteFile/CreateDirectory/DeleteDirectory/RenameFile/
RenameDirectory` and `fsFileWrite/SetSize`, called only by `fsdev` (behind the
firewall) on the only file system opened, `fsOpenSdCardFileSystem`. `setsys`
is only read (`setsysGetFirmwareVersion`). `appletSetCpuBoostMode` /
`apmSetPerformanceConfiguration` are referenced by libounce's `applet.c` to
restore the defaults (checked by `tests/check_no_overclock.sh`).

## 3. Layer 2 — devoptab firewall (`nx2_fsguard.c`)

Every file function of newlib goes through the `devoptab_list` table. In
`userAppInit` (called by libounce right after mounting the SD card, **before
any C++ constructor and before any file is opened**) the `sdmc` entry is
replaced with a copy whose operations below consult the policy before calling
fsdev:

| newlib operation | Check |
|---|---|
| `open_r` with `O_WRONLY`/`O_RDWR`/`O_CREAT`/`O_TRUNC`/`O_APPEND` | file write |
| `unlink_r`, `chmod_r`, `utimes_r` | file write |
| `rename_r` (source **and** destination), `link_r`, `symlink_r` | file write |
| `mkdir_r` | directory creation (the sandbox directory itself is allowed) |
| `rmdir_r` | directory removal |
| `chdir_r` | only inside the sandbox (relative paths therefore always start there) |
| `write_r`, `ftruncate_r`, `fchmod_r`, `fsync_r` | on a descriptor that only `G_open` could open for writing |

A refusal sets `errno = EACCES`, the call never reaches fsdev, and the event is
recorded and written to `logs/rdoom.log` (category `fsguard`). If installation
fails (no SD card, folder cannot be created), an error screen is shown and
**nothing starts**. If a component registered `sdmc` again, the firewall is
put back (`NX2_FsGuardActive`, checked by the writer thread). `/switch` is
never created; the `switch/uzdoom-nx2` folder itself cannot be written,
removed or renamed.

## 4. Layer 3 — path policy (`nx2_pathpolicy.c`)

Resolves the path as fsdev does (optional `sdmc:` prefix, relative to the
working directory tracked by the firewall), normalises `.`/`..`, then requires
the first two components to be `switch` and `uzdoom-nx2` (case-insensitive,
like FAT/exFAT). Always refused: any other prefix (`bis:`, `save:`, `SDMC:`…),
several `:`, `\`, control characters and wildcards, names made of dots/spaces
or ending in `.`/space, `..` above the root, paths longer than 0x300. 54 tests
(`tests/pathpolicy_tests.c`, ASan+UBSan) in `scripts/test.sh`.

## 5. What the program writes

All relative to the working directory (= `sdmc:/switch/uzdoom-nx2`) or
absolute under `NX2_SANDBOX_ROOT`.

| Origin | Files |
|---|---|
| Start-up (`rdoom_nx.cpp`) | `logs/` (rotation `logs/anterior*`), `boot.log`, `rdoom.log`, `engine.log`, `engine-stderr.log`, `engine-session.txt`, `gpu-*.txt` |
| Crashes (`i_nxsystem.cpp`, `nx_glthread.cpp`) | `logs/crash.log`, `hang.log`, `logs/phase.log` |
| Measurements (`nx_perf.cpp`, `rdoom_gpubench.cpp`) | `logs/perf.log`, `logs/gpu-bench.txt` |
| Launcher (`launcher/src/core`) | `profiles/*.json` (+ `.bak`, atomic writes), `config/launcher.json`, `config/engine/<profile>.ini`, `backup/`, `export/`, `cache/` (thumbnails, `loadinfo.txt`), `logs/last-launch.txt` |
| UZDoom engine | `config/engine/<profile>.ini`, `saves/<profile>/`, `screenshots/`, `demo/`, `cache/uzdoom/` (Vulkan pipelines, GL nodes) |
| Loading screen | `loadtimes.txt` (load time history) |
| UZDoom console (`logfile`, `dumpjit`…) | relative to the folder; a path outside it is refused by the firewall |

Removed compared with RDoom: deleting `sdmc:/.mesa/mesa_shader_cache_sf` (at
start-up and from the "Clear shader cache" button).

## 6. Outside the program's control

- Screenshots and videos taken with the Capture button: handled by the system
  (as for any game), not by the homebrew.
- kaiHEN, hbloader and hbmenu themselves.
