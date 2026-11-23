# Build UZDoom NX2

These instructions target a Linux environment, including WSL 2 with Docker Desktop integration enabled.

## Requirements

- Git
- Docker Engine accessible from the Linux or WSL environment
- Python 3
- ImageMagick with the `magick` command
- `curl` and `sha256sum`

The build script creates build output locally. Build artifacts are ignored by Git.

## 1. Prepare the toolchain image

Clone the [ounce-devkit](https://github.com/ouncebrew/ounce-devkit) project and build its base image:

```sh
git clone https://github.com/ouncebrew/ounce-devkit.git ../ounce-devkit
docker build -t ounce-devkit ../ounce-devkit
```

Run these commands from the UZDoom NX2 repository root. If you already have the `ounce-devkit` image, you can skip this step.

## 2. Reconstruct the engine source

The engine checkout is not stored in this repository. Recreate it from the pinned UZDoom 5.0.3 revision and the patch series:

```sh
git clone https://github.com/UZDoom/UZDoom.git engine
git -C engine checkout -b uzdoom-nx2 7910df7102033fd7754a317f3254f24101135926
git -C engine am ../patches/*.patch
```

The patch checksums are listed in `patches/SHA256SUMS.txt`.

## 3. Build the project

From the repository root, run:

```sh
scripts/build.sh
```

The script downloads and verifies the pinned OunceVK package when needed, builds the project image and host tools, generates launcher artwork, builds the homebrew, runs the NAND safety checks and host tests, and creates the release package.

To limit parallel jobs:

```sh
JOBS=8 scripts/build.sh
```

To clear the native and Switch build directories before building:

```sh
CLEAN=1 scripts/build.sh
```

The main outputs are:

- `dist/uzdoom-nx2.nro`
- `dist/UZDoom-NX2-<version>.zip`
- `dist/UZDoom-NX2/`, the extracted SD card package

The release ZIP is intended to be extracted to the SD card root. See [Installation and usage](INSTALL.txt).

## Host checks

The full build runs `scripts/test.sh` after packaging. To run the host tests separately:

```sh
scripts/test.sh
```

These automated checks do not replace testing on a physical console. Do not interpret a successful host build as proof that every game or mod works on hardware.

## Source and licensing

The exact UZDoom base revision and the port changes are available through the public upstream repository and this repository's patch series. See [patches/README.md](../patches/README.md) for the source reconstruction details.

See [LICENSE](../LICENSE) and [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) for the project and third-party licensing information. The source tree includes only the Freedoom IWADs as game data; do not add commercial IWADs, third-party mods, or credentials to a public commit.
