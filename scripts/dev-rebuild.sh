#!/usr/bin/env bash
# Incremental rebuild of uzdoom-nx2.nro + NAND safety check + package
# (development loop; full build: scripts/build.sh).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
run() { docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp -v "$ROOT:/src" -w /src uzdoom-nx2-build bash -c "$1"; }
run 'export PORTLIBS_PREFIX=$DEVKITPRO/portlibs/switch; ninja -C build/nx2 -j"$(nproc)"' > "$ROOT/build/logs/nx2-dev.log" 2>&1 \
	|| { grep -E "error|undefined|multiple" "$ROOT/build/logs/nx2-dev.log" | head -20; exit 1; }
run 'tests/check_nand_safety.sh build/nx2/uzdoom.elf' > "$ROOT/build/logs/nand-safety.log" 2>&1 \
	|| { cat "$ROOT/build/logs/nand-safety.log"; rm -f "$ROOT/build/nx2/uzdoom-nx2.nro"; echo "NAND safety check failed: .nro deleted" >&2; exit 1; }
"$ROOT/scripts/package.sh" > /dev/null
echo "rebuilt: $(sha256sum "$ROOT/dist/uzdoom-nx2.nro")"
