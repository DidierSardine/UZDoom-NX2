# UZDoom NX2

An unofficial Nintendo Switch 2 homebrew port of [UZDoom](https://github.com/UZDoom/UZDoom), built for kaiHEN and rendered with Vulkan through OunceVK. It includes a launcher for profiles, IWADs, and mods, with English, French, and Portuguese translations.

<img width="1920" height="1080" alt="2026112322355700" src="https://github.com/user-attachments/assets/47fa2215-d4db-4141-80c9-13fc76928c8a" />
<img width="1920" height="1080" alt="2026112322352800" src="https://github.com/user-attachments/assets/4b41aa4a-f8f9-4d5f-874a-6a5f7354935e" />
<img width="1920" height="1080" alt="2026112322364400" src="https://github.com/user-attachments/assets/b9fed213-a647-47f8-b245-5783c8790583" />


**Project status:** experimental. Compatibility and performance vary by game, mod, and console setup.

## AI development disclosure

This project was vibe-coded with AI assistance.

## Known issues

Some mods are not functional yet, such as myhouse.wad. Mods such as Brutal Doom and Reelism have been tested and work.

## What is included

- UZDoom 5.0.3 and the Switch 2 port patch series.
- The launcher and build scripts.
- Freedoom Phase 1 and Phase 2 IWADs, the only IWAD files included in this repository.
- Required UZDoom runtime data is produced from the engine source during the build; it is not checked into this repository.

Commercial IWADs and third-party mods are not included. Add only files you are entitled to use.

## Switch 2 features

You can use the Gyroscope to aim. The setting is in Controller Options.

The Haptics also work as well !

## Install and play

For SD card setup, launch steps, and controls, see [Installation and usage](docs/INSTALL.txt).

The prebuilt package is produced at `dist/UZDoom-NX2-<version>.zip` by the build process. Extract it to the root of the SD card, then launch UZDoom NX2 from hbmenu under kaiHEN.

## Build from source

See [Build instructions](docs/BUILD.md) for the required tools, engine source setup, and build commands. The engine source is reconstructed from the pinned upstream UZDoom revision and the patch series in [patches/](patches/README.md).

## Safety

The port is designed to keep its file writes inside `sdmc:/switch/uzdoom-nx2`. The project includes build-time checks and a filesystem guard. Read [the security notes](docs/SECURITY.md) for details and limitations.

## Licensing and credits

The project's software is distributed under GPL version 3; see [LICENSE](LICENSE). Third-party components and assets have their own terms. [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) lists the components, licenses, and attributions.

Freedoom is distributed under its BSD-style license. Its full license and contributor credits are in [third_party/freedoom/](third_party/freedoom/). UZDoom branding is credited to The UZDoom Team and designer Carlos "Cardboard Marty" Sanchez under CC BY-SA 4.0. RDoom NX is credited to Renan (Æon). See the notices for additional upstream authors and component credits.

UZDoom NX2 is an independent project. It is not affiliated with id Software, Bethesda, Nintendo, or the UZDoom team. "Doom" is a trademark of its respective owner.
