# Third-party notices — UZDoom NX2

UZDoom NX2 is based on RDoom NX by Renan (Æon) (GPLv3, github.com/aeon1911/RDoom-NX).

UZDoom NX2 is distributed under the **GNU General Public License v3** (the licence
of UZDoom, on which it is based). The full text is included in this repository as `LICENSE` (and packaged as
`switch/uzdoom-nx2/licenses/GPL-3.0-UZDoom.txt`). The `engine/` checkout is
reconstructed locally and ignored by Git. Its exact upstream base revision and
the complete RDoom NX / UZDoom NX2 patch series are published in `patches/`;
see `patches/README.md` and `docs/BUILD.md` to recreate it. The launcher, build
scripts, and project documentation are tracked here.

UZDoom NX2 is an unofficial port. It is not a product of id Software, Bethesda,
ZeniMax, Nintendo, or the UZDoom/GZDoom maintainers. "Doom" is a trademark of
its owners; no Doom logo, character art or commercial game data is included.

## Components linked into uzdoom-nx2.nro

| Component | Version / revision | Licence | Where |
|---|---|---|---|
| UZDoom | tag 5.0.3, `7910df7102033fd7754a317f3254f24101135926` | GPL-3.0-or-later (engine), with code under BSD/MIT/zlib and others listed in `engine/docs/licenses/` | `engine/` |
| GZDoom / ZDoom lineage | via UZDoom | GPL-3.0-or-later and the historic Doom source licence terms in UZDoom | `engine/` |
| Earlier Switch port of GZDoom (fgsfds) — platform code, crash handler, loading screen base | via the user's UZDoom 5.0.3 Switch port | BSD-3-Clause style (file headers) / GPL-3.0 | `engine/src/common/platform/posix/nx/` |
| OunceVK (Mesa NVK for Switch 2: Vulkan, Zink/EGL linked for SDL2, not used for rendering) | ouncevk 20261118 (GitHub release package, `git-2d6a8aec8b`; github.com/ouncebrew/ouncevk) | MIT (most of Mesa; see `MESA-MIT.txt`), other components per Mesa's licences | linked static |
| Rust standard library (inside OunceVK, used by NAK) | as built by OunceVK | MIT OR Apache-2.0 | linked static |
| libounce (libnx for Switch 2, includes mimalloc) | 20261117.1 (github.com/ouncebrew/libounce) | ISC (`LIBNX-ISC.txt`); mimalloc MIT | linked static |
| devkitA64 runtime (newlib, libgcc, libstdc++) | GCC 15.2.0 toolchain | newlib: BSD-style licences; libgcc/libstdc++: GPL-3.0 with the GCC Runtime Library Exception | linked static |
| SDL2 (switch-sdl2) | 2.28.5 | zlib | linked static |
| OpenAL Soft (switch-openal-soft) | 1.21.1 | LGPL-2.1-or-later | linked static (satisfied by full source availability) |
| libsndfile | 1.2.2 (`72f6af15e8f85157bd622ed45b979025828b7001`) | LGPL-2.1-or-later | linked static |
| libogg | 1.3.4 (devkitPro switch-libogg) | BSD-3-Clause (Xiph.Org) | linked static (libsndfile: Ogg sounds) |
| libvorbis / libvorbisenc | 1.3.7 (devkitPro switch-libvorbis) | BSD-3-Clause (Xiph.Org) | linked static (libsndfile: Vorbis sounds) |
| libFLAC | 1.3.2 (devkitPro switch-flac) | BSD-3-Clause (Xiph.Org) | linked static (libsndfile: FLAC sounds) |
| libopus | 1.3 (devkitPro switch-libopus) | BSD-3-Clause (Xiph.Org, Skype, others) | linked static (libsndfile: Opus sounds) |
| mpg123 | 1.31.3 | LGPL-2.1 | linked static |
| libogg / libvorbis / FLAC / Opus / opusfile | devkitPro portlibs | BSD-3-Clause | linked static |
| zlib, zstd, expat | devkitPro portlibs | zlib / BSD-3-Clause / MIT | linked static |
| ZMusic, FluidSynth, TiMidity++, libADLMIDI, libOPNMIDI, WildMidi | bundled in UZDoom 5.0.3 | LGPL/GPL/zlib/BSD as listed in `engine/libraries/ZMusic/licenses/` | linked static |
| ZVulkan, VMA, glslang, volk (bundled with UZDoom) | UZDoom 5.0.3 | MIT / BSD-3-Clause / Apache-2.0 (glslang parts) | linked static |
| nlohmann/json | 3.11.3 (sha256 `9bea4c80…103ea6`) | MIT (`NLOHMANN-JSON-MIT.txt`) | `launcher/third_party/json.hpp` |
| stb_image 2.30, stb_truetype 1.26, stb_image_write 1.16 | nothings/stb | MIT or Public Domain (`STB-LICENSE.txt`) | `launcher/third_party/` |
| Bebas Neue, Poppins (fonts) | from the earlier Switch port | SIL Open Font License 1.1 (`OFL-BebasNeue.txt`, `OFL-Poppins.txt`) | embedded in the launcher |

## Shipped data

| Data | Licence | Notes |
|---|---|---|
| `uzdoom.pk3`, `game_support.pk3`, `game_widescreen_gfx.pk3`, `brightmaps.pk3`, `lights.pk3`, `soundfonts/uzdoom.sf2`, `fm_banks/*` | as in UZDoom 5.0.3 | built unchanged from `engine/wadsrc*` |
| UZDoom logo (icon `uzdoom-nx2.nro`, `launcher/logo_*.png`, `launcher/splash.jpg`) | **CC BY-SA 4.0**, © 2025 The UZDoom Team, logo designed by Carlos "Cardboard Marty" Sanchez (`licenses/uzdoom-branding/`) | derived from `engine/branding/` by `assets/src/make_art.py` (scaled; banner recoloured light); the derived images are under the same licence |
| Other launcher images (`bg_*.jpg`, `card_*.jpg`) | GPL-3.0 | gradients and Bebas Neue lettering (SIL OFL), `assets/src/make_art.py` |
| Freedoom 0.13.0 `freedoom1.wad`, `freedoom2.wad` (in `switch/uzdoom-nx2/iwads/`, and in `third_party/freedoom/`) | BSD-3-Clause style (`licenses/freedoom/COPYING.txt`), credits in `CREDITS.txt` | official release; sha256 checked by the build (`third_party/freedoom/SHA256SUMS.txt`) |

No commercial IWAD, mod or extracted game asset is distributed. Users supply
their own IWADs and mods; `tests/check_package.py` rejects any IWAD in the
package other than the two Freedoom files above.

## Tools used to build (not distributed)

devkitPro `devkita64:20260219` Docker image (digest
`sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`),
CMake, Ninja, rsvg-convert, ImageMagick, glslc (vktest shaders).
