#!/usr/bin/env bash
# Assembles the SD card package from build/nx2 and assets/build.
#
#   dist/UZDoom-NX2/                      tree to copy to the SD card root
#   dist/uzdoom-nx2.nro                   the homebrew alone
#   dist/UZDoom-NX2-<ver>.zip             extract to the SD card root
#   dist/UZDoom-NX2-<ver>-Freedoom.zip    same + Freedoom IWADs (BSD), if third_party/freedoom exists
#
# Everything goes to switch/uzdoom-nx2/: the program never writes elsewhere.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
B="$ROOT/build/nx2"
VER=$(sed -n 's/.*set( RDOOM_VERSION "\([^"]*\)" ).*/\1/p' "$ROOT/engine/CMakeLists.txt")
OUT="$ROOT/dist/UZDoom-NX2"
SD="$OUT/switch/uzdoom-nx2"

[ -f "$B/uzdoom-nx2.nro" ] || { echo "build/nx2/uzdoom-nx2.nro missing: run scripts/build.sh" >&2; exit 1; }
# never package a binary that failed the NAND safety check
grep -q "^ok: no NAND" "$ROOT/build/logs/nand-safety.log" 2>/dev/null \
	|| { echo "NAND safety check has not passed for this build: not packaging" >&2; exit 1; }

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

cat > "$SD/iwads/LISEZ-MOI.txt" <<'EOF'
Placez ici les IWAD (jeux de base) que vous possedez :
  freedoom1.wad, freedoom2.wad (libres : https://freedoom.github.io)
  doom.wad, doom2.wad, plutonia.wad, tnt.wad, heretic.wad, hexen.wad ... (commerciales : copiez les votres)
  doom_fusion.ipk3 (WadFusion, genere a partir de vos fichiers)
UZDoom NX2 ne fournit aucune IWAD commerciale.
EOF
cat > "$SD/mods/LISEZ-MOI.txt" <<'EOF'
Placez ici les mods et packs de cartes (.pk3 .wad .zip .pk7 .deh .bex .ipk3).
Les sous-dossiers sont autorises (ex. : mods/lexicon/lexicon-core-v1.0.pk3).
Rien ici n'est jamais efface : retirer un mod d'un profil ne fait que retirer la reference.
EOF

# licences
cp "$ROOT/engine/LICENSE" "$SD/licenses/GPL-3.0-UZDoom.txt"
cp -r "$ROOT/engine/docs/licenses" "$SD/licenses/uzdoom-third-party"
cp "$ROOT/THIRD_PARTY_NOTICES.md" "$SD/licenses/"
cp "$ROOT/launcher/res/fonts/OFL-BebasNeue.txt" "$ROOT/launcher/res/fonts/OFL-Poppins.txt" "$SD/licenses/"
cp "$ROOT/third_party/licenses/"* "$SD/licenses/" 2>/dev/null || true

{
	echo "UZDoom NX2 $VER (Nintendo Switch 2, kaiHEN)"
	echo "build: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
	echo "engine: UZDoom 5.0.3 (7910df7102033fd7754a317f3254f24101135926) + RDoom NX + UZDoom NX2 patches $(git -C "$ROOT/engine" describe --always --dirty --tags)"
	echo "graphics: OunceVK (Mesa NVK for Switch 2), Vulkan; system library: libounce"
	echo "uzdoom-nx2.nro sha256: $(sha256sum "$SD/uzdoom-nx2.nro" | cut -d' ' -f1)"
	echo "writes: only sdmc:/switch/uzdoom-nx2 (SD firewall); no NAND access (checked at build time)"
	echo "clocks: stock only"
} > "$SD/VERSION.txt"
cp "$ROOT/docs/INSTALL-fr.txt" "$OUT/LISEZ-MOI.txt"

cp "$SD/uzdoom-nx2.nro" "$ROOT/dist/uzdoom-nx2.nro"
python3 -I "$ROOT/scripts/mkzip.py" "$ROOT/dist/UZDoom-NX2-$VER.zip" "$OUT" switch LISEZ-MOI.txt

FD="$ROOT/third_party/freedoom/freedoom-0.13.0"
if [ -f "$FD/freedoom2.wad" ]; then
	TMP=$(mktemp -d "$ROOT/build/pkg-XXXX")
	cp -r "$OUT/switch" "$OUT/LISEZ-MOI.txt" "$TMP/"
	cp "$FD/freedoom1.wad" "$FD/freedoom2.wad" "$TMP/switch/uzdoom-nx2/iwads/"
	mkdir -p "$TMP/switch/uzdoom-nx2/licenses/freedoom"
	cp "$FD/COPYING.txt" "$FD/CREDITS.txt" "$FD/CREDITS-MUSIC.txt" "$TMP/switch/uzdoom-nx2/licenses/freedoom/"
	python3 -I "$ROOT/scripts/mkzip.py" "$ROOT/dist/UZDoom-NX2-$VER-Freedoom.zip" "$TMP" switch LISEZ-MOI.txt
	rm -rf "$TMP"
fi
(cd "$ROOT/dist" && sha256sum uzdoom-nx2.nro UZDoom-NX2-*.zip > SHA256SUMS.txt)
echo "package: $OUT"
ls -l "$ROOT/dist"
