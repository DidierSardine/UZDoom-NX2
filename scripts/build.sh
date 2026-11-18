#!/usr/bin/env bash
# UZDoom NX2 - full build (Nintendo Switch 2, kaiHEN).
#
#   scripts/build.sh            # image + host tools + uzdoom-nx2.nro + safety checks + package
#   JOBS=8 scripts/build.sh     # limit parallel jobs (default: nproc)
#   CLEAN=1 scripts/build.sh    # start from empty build directories
#
# Needs the ounce-devkit image (https://github.com/ouncebrew/ounce-devkit):
#   docker build -t ounce-devkit <ounce-devkit checkout>
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
JOBS=${JOBS:-$(nproc)}
IMAGE=uzdoom-nx2-build
LOG_DIR="$ROOT/build/logs"
mkdir -p "$LOG_DIR"

step() { printf '\n==> %s\n' "$*"; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

for t in docker python3 magick sha256sum; do command -v "$t" >/dev/null || die "missing host tool: $t"; done
docker info >/dev/null 2>&1 || die "docker daemon not reachable"
docker image inspect ounce-devkit >/dev/null 2>&1 || die "build the ounce-devkit image first (see header)"

step "Freedoom 0.13.0 (shipped in the package)"
(cd "$ROOT/third_party/freedoom" && sha256sum -c --quiet SHA256SUMS.txt) || die "third_party/freedoom: checksum mismatch"

step "Toolchain image ($IMAGE)"
docker build -q -f "$ROOT/docker/Dockerfile" -t "$IMAGE" "$ROOT/docker" > "$LOG_DIR/docker-image.log" 2>&1 \
	|| { cat "$LOG_DIR/docker-image.log"; die "docker build failed"; }

if [ "${CLEAN:-0}" = 1 ]; then
	rm -rf "$ROOT/build/native" "$ROOT/build/nx2"
fi

run() {
	docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp -e JOBS="$JOBS" \
		-v "$ROOT:/src" -w /src "$IMAGE" bash -euo pipefail -c "$1"
}

step "Host tools (zipdir, re2c, lemon)"
run 'cmake -S engine -B build/native -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
     ninja -C build/native -j"$JOBS" zipdir re2c lemon' > "$LOG_DIR/native.log" 2>&1 \
	|| { tail -40 "$LOG_DIR/native.log"; die "host tools failed"; }

step "Icon and launcher art"
python3 "$ROOT/assets/src/make_art.py" "$ROOT/assets/build" > "$LOG_DIR/icon.log" 2>&1 \
	|| { cat "$LOG_DIR/icon.log"; die "icon failed"; }

step "uzdoom-nx2.nro (UZDoom 5.0.3 + launcher, Vulkan via OunceVK, libounce)"
run 'S1=$DEVKITPRO/portlibs/switch; S2=$DEVKITPRO/portlibs/switch2
     export PORTLIBS_PREFIX=$S1
     if [ ! -f build/nx2/build.ninja ]; then
       cmake -S engine -B build/nx2 -G Ninja \
         -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/libounce/cmake/Switch2.cmake \
         -DCMAKE_BUILD_TYPE=Release -DNINTENDO_SWITCH=ON \
         -DHAVE_VULKAN=ON -DHAVE_OPENGL=OFF -DHAVE_GLES2=OFF -DNO_STRIP=OFF \
         -DDYN_OPENAL=OFF -DDYN_SNDFILE=OFF -DDYN_MPG123=OFF \
         -DIMPORT_EXECUTABLES=/src/build/native/ImportExecutables.cmake \
         -DSDL2_INCLUDE_DIR=$S1/include/SDL2 -DSDL2_LIBRARY=$S1/lib/libSDL2.a \
         -DSNDFILE_INCLUDE_DIR=$S2/include -DSNDFILE_LIBRARY=$S2/lib/libsndfile.a \
         -DRDOOM_ICON=/src/assets/build/icon_256.jpg
     fi
     ninja -C build/nx2 -j"$JOBS"' > "$LOG_DIR/nx2.log" 2>&1 \
	|| { grep -E "error|Error|undefined" "$LOG_DIR/nx2.log" | head -60; tail -20 "$LOG_DIR/nx2.log"; die "Switch 2 build failed (full log: $LOG_DIR/nx2.log)"; }
echo "compiler warnings: $(grep -c "warning:" "$LOG_DIR/nx2.log" || true) (build/logs/nx2.log)"

step "NAND safety checks"
run 'tests/check_nand_safety.sh build/nx2/uzdoom.elf' > "$LOG_DIR/nand-safety.log" 2>&1 \
	|| { cat "$LOG_DIR/nand-safety.log"; rm -f "$ROOT/build/nx2/uzdoom-nx2.nro"; die "NAND safety check failed: the .nro was deleted and must not be used"; }
cat "$LOG_DIR/nand-safety.log"

step "Packaging"
"$ROOT/scripts/package.sh"

step "Host tests"
"$ROOT/scripts/test.sh"

step "Done"
ls -l "$ROOT/dist/uzdoom-nx2.nro" "$ROOT/dist/"*.zip
