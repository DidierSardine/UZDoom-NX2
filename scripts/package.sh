#!/usr/bin/env bash
# Assembles the SD card package from build/nx2, assets/build and third_party.
#
#   dist/UZDoom-NX2/                tree to copy to the SD card root
#   dist/uzdoom-nx2.nro             the homebrew alone
#   dist/UZDoom-NX2-<ver>.zip       extract to the SD card root (Freedoom included)
#
# Everything goes to switch/uzdoom-nx2/: the program never writes elsewhere.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
B="$ROOT/build/nx2"
VER=$(sed -n 's/.*set( RDOOM_VERSION "\([^"]*\)" ).*/\1/p' "$ROOT/engine/CMakeLists.txt")
OUT="$ROOT/dist/UZDoom-NX2"
SD="$OUT/switch/uzdoom-nx2"
FD="$ROOT/third_party/freedoom"

[ -f "$B/uzdoom-nx2.nro" ] || { echo "build/nx2/uzdoom-nx2.nro missing: run scripts/build.sh" >&2; exit 1; }
# never package a binary that failed the NAND safety check
grep -q "^ok: no NAND" "$ROOT/build/logs/nand-safety.log" 2>/dev/null \
	|| { echo "NAND safety check has not passed for this build: not packaging" >&2; exit 1; }
(cd "$FD" && sha256sum -c --quiet SHA256SUMS.txt) || { echo "third_party/freedoom: checksum mismatch" >&2; exit 1; }

rm -rf "$OUT" "$ROOT/dist/uzdoom-nx2.nro" "$ROOT"/dist/UZDoom-NX2-*.zip "$ROOT/dist/SHA256SUMS.txt"
mkdir -p "$SD"/{iwads,mods,launcher,licenses}

cp "$B/uzdoom-nx2.nro" "$SD/"
for pk3 in uzdoom.pk3 game_support.pk3 game_widescreen_gfx.pk3 brightmaps.pk3 lights.pk3; do
	cp "$B/$pk3" "$SD/"
done
cp -r "$B/soundfonts" "$B/fm_banks" "$SD/"

for f in bg_main.jpg bg_inner.jpg splash.jpg logo_300.png logo_400.png logo_620.png \
         card_freedoom2.jpg card_brutaldoom.jpg card_brutality.jpg card_wadfusion.jpg card_lexicon.jpg card_compendium.jpg card_custom.jpg; do
	cp "$ROOT/assets/build/$f" "$SD/launcher/"
done

# Freedoom (BSD): playable out of the box
cp "$FD/freedoom1.wad" "$FD/freedoom2.wad" "$SD/iwads/"
mkdir -p "$SD/licenses/freedoom"
cp "$FD/COPYING.txt" "$FD/CREDITS.txt" "$FD/CREDITS-MUSIC.txt" "$SD/licenses/freedoom/"

printf '%s\n' \
	"IWADs (base games) go here." \
	"  freedoom1.wad, freedoom2.wad: free, included (https://freedoom.github.io)" \
	"  doom.wad, doom2.wad, plutonia.wad, tnt.wad, heretic.wad, hexen.wad ...: commercial, copy your own" \
	"  doom_fusion.ipk3 (WadFusion, generated from your own files)" \
	"UZDoom NX2 does not ship any commercial IWAD." > "$SD/iwads/README.txt"
printf '%s\n' \
	"Mods and map packs go here (.pk3 .wad .zip .pk7 .deh .bex .ipk3)." \
	"Subfolders are allowed (e.g. mods/lexicon/lexicon-core-v1.0.pk3)." \
	"Nothing here is ever deleted: removing a mod from a profile only removes the reference." > "$SD/mods/README.txt"

# licences
cp "$ROOT/engine/LICENSE" "$SD/licenses/GPL-3.0-UZDoom.txt"
cp -r "$ROOT/engine/docs/licenses" "$SD/licenses/uzdoom-third-party"
cp "$ROOT/THIRD_PARTY_NOTICES.md" "$SD/licenses/"
cp "$ROOT/launcher/res/fonts/OFL-BebasNeue.txt" "$ROOT/launcher/res/fonts/OFL-Poppins.txt" "$SD/licenses/"
cp "$ROOT/third_party/licenses/"* "$SD/licenses/" 2>/dev/null || true
# official UZDoom branding (the icon and launcher logos are derived from it)
mkdir -p "$SD/licenses/uzdoom-branding"
cp "$ROOT/engine/branding/BRANDING-LICENSE.md" "$SD/licenses/uzdoom-branding/"
cp "$ROOT/engine/wadsrc/static/credits/UZDoom Logo.txt" "$SD/licenses/uzdoom-branding/"

{
	echo "UZDoom NX2 $VER (Nintendo Switch 2, kaiHEN)"
	echo "build: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
	echo "engine: UZDoom 5.0.3 (official tag 5.0.3, 7910df7102033fd7754a317f3254f24101135926) + RDoom NX + UZDoom NX2 patches $(git -C "$ROOT/engine" describe --always --dirty --tags)"
	echo "graphics: OunceVK (Mesa NVK for Switch 2), Vulkan; system library: libounce"
	echo "uzdoom-nx2.nro sha256: $(sha256sum "$SD/uzdoom-nx2.nro" | cut -d' ' -f1)"
	echo "writes: only sdmc:/switch/uzdoom-nx2 (SD firewall); the NAND is never opened (checked at build time)"
	echo "clocks: stock only"
} > "$SD/VERSION.txt"
cp "$ROOT/docs/INSTALL.txt" "$OUT/README.txt"

cp "$SD/uzdoom-nx2.nro" "$ROOT/dist/uzdoom-nx2.nro"
python3 -I "$ROOT/scripts/mkzip.py" "$ROOT/dist/UZDoom-NX2-$VER.zip" "$OUT" switch README.txt
(cd "$ROOT/dist" && sha256sum uzdoom-nx2.nro UZDoom-NX2-*.zip > SHA256SUMS.txt)
echo "package: $OUT"
ls -l "$ROOT/dist"
