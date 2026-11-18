#!/usr/bin/env bash
# UZDoom NX2 - NAND safety checks. Fails (exit 1) on any finding.
#
#   tests/check_nand_safety.sh <unstripped .elf> [nm]
#
# 1. Binary: the final, linked ELF (static, --gc-sections: only code that is
#    reachable is in it) must not contain any libnx/libounce/libkaihen entry
#    point that opens NAND, save data, system storages, or that changes
#    system settings, installed content, tickets, pairing data, clocks or
#    hardware. kaiHEN does not restrict homebrew, so this list is the
#    guarantee that the program cannot write anything but the SD card.
# 2. Binary: the SD card firewall (nx2_fsguard.c) must be linked.
# 3. Sources: the port's own code must not talk to system services directly
#    (raw IPC) nor use the FS API below the C library (fsFs*, fsFile*...),
#    which would bypass the firewall.
set -euo pipefail
ELF=${1:?usage: check_nand_safety.sh <elf> [nm]}
NM=${2:-aarch64-none-elf-nm}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
fail=0

syms=$("$NM" "$ELF" | awk '{print $NF}' | sort -u)

# Symbol name patterns (extended regex, anchored) that must not be linked.
deny=(
	# fs: NAND partitions, save data, system storages, content, game card
	'fsOpenBis.*' 'fsOpenSaveData.*' 'fsOpen_.*' 'fsOpenSystemSaveData.*'
	'fsCreate.*' 'fsDelete.*' 'fsExtend.*' 'fsFormat.*' 'fsCorrupt.*'
	'fsOpenContentStorage.*' 'fsOpenCustomStorage.*' 'fsOpenImageDirectory.*'
	'fsOpenGameCard.*' 'fsOpenFileSystem.*' 'fsOpenDeviceOperator.*' 'fsDeviceOperator.*'
	'fsOpenHostFileSystem.*' 'fsOpenRegisteredUpdatePartition.*' 'fsOpenCodeFileSystem.*'
	'fsSetSaveData.*' 'fsWriteSaveData.*' 'fsSetBisRoot.*' 'fsSetSdCardEncryption.*'
	'fsSetCurrentPosixTime.*' 'fsRegister.*' 'fsUnregister.*' 'fsDisableAutoSaveDataCreation.*'
	'fsSetGlobalAccessLogMode.*' 'fsOpenSaveDataTransfer.*' 'fsldr.*'
	# fsdev: mounts of anything but the SD card, raw access below the firewall
	'fsdevMountSaveData.*' 'fsdevMountSystemSaveData.*' 'fsdevMountBcatSaveData.*'
	'fsdevMountDeviceSaveData.*' 'fsdevMountTemporaryStorage.*' 'fsdevMountCacheStorage.*'
	'fsdevMountSystemBcatSaveData.*' 'fsdevGetDeviceFileSystem' 'fsdevDeleteDirectoryRecursively'
	'fsdevCreateFile' 'fsdevSetConcatenationFileAttribute' 'fsdevCommitDevice' 'fsdevTranslatePath'
	# system settings, calibration, installed content, tickets, programs
	'setsysSet.*' 'setsysReset.*' 'setcal.*' 'setsysBind.*'
	'ns[A-Z].*' 'ncm[A-Z].*' 'es[A-Z][a-z].*' 'lr[A-Z].*' 'spl[A-Z].*'
	'pmshell.*' 'pmdmnt.*' 'pmbm.*' 'ldrPm.*' 'ldrShell.*' 'ldrDmnt.*'
	# Bluetooth pairing database, album, network profiles, parental controls
	'btm[A-Z].*' 'btmsys.*' 'btdrv.*' 'caps[a-z]*[A-Z].*' 'nifmSet.*' 'pctl[A-Z].*'
	# clocks, power, hardware buses
	# (apmSetPerformanceConfiguration and appletSetCpuBoostMode are referenced by
	# libnx's own applet.c to restore the defaults; tests/check_no_overclock.sh
	# checks that nothing else calls them.)
	'clkrst.*' 'pcv[A-Z].*' 'i2c[A-Z].*' 'gpio[A-Z].*' 'pinmux.*' 'fan[A-Z].*'
	'timeSet.*'
	# kernel calls that reach other processes or the secure monitor
	'svcCallSecureMonitor' 'svcWriteDebugProcessMemory' 'svcDebugActiveProcess'
	# kaiHEN's homebrew API (FS root clones, loader root, plugins)
	'kaihen.*'
)

for pat in "${deny[@]}"; do
	hits=$(grep -E "^${pat}\$" <<<"$syms" || true)
	if [ -n "$hits" ]; then
		echo "NAND-SAFETY FAIL: forbidden symbol(s) linked (pattern ${pat}):"
		sed 's/^/    /' <<<"$hits"
		fail=1
	fi
done

# The firewall's entry point, its policy and every guarded operation must be
# in the binary (LTO may suffix local names: ".lto_priv.0").
for need in NX2_FsGuardInstall NX2_PathCheck G_open G_unlink G_rename G_mkdir G_rmdir G_chdir; do
	if ! grep -qE "^${need}(\.[a-z_0-9.]+)?\$" <<<"$syms"; then
		echo "NAND-SAFETY FAIL: the SD card firewall is not linked ($need missing)"
		fail=1
	fi
done
# ...and userAppInit (run by libounce right after mounting the SD card, before
# any constructor) must call NX2_FsGuardInstall.
OBJDUMP=${NM%nm}objdump
if ! "$OBJDUMP" -d --no-show-raw-insn --disassemble=userAppInit "$ELF" 2>/dev/null | grep -qE "bl\s+[0-9a-f]+ <NX2_FsGuardInstall"; then
	echo "NAND-SAFETY FAIL: userAppInit does not call NX2_FsGuardInstall"
	fail=1
fi

# Sources written for this port (engine platform code, launcher, port-only
# engine files). Raw IPC or FS calls below newlib are not allowed in them.
src_dirs=("$ROOT/engine/src/common/platform/posix/nx" "$ROOT/launcher/src")
src_files=$(find "${src_dirs[@]}" -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' \) ! -name 'nx_font_*.h')
raw='\b(serviceDispatch[A-Za-z]*|smGetService[A-Za-z]*|svcSendSyncRequest[A-Za-z]*|svcConnectToNamedPort|fsFs[A-Z][A-Za-z]*|fsFile[A-Z][A-Za-z]*|fsDir[A-Z][A-Za-z]*|fsOpen[A-Za-z_]*|fsdev(Mount|Unmount|Get|Delete|Create|Commit|Translate)[A-Za-z]*|fsInitialize|romfsMount[A-Za-z]*)\s*\('
if hits=$(grep -nE "$raw" $src_files); then
	echo "NAND-SAFETY FAIL: direct system/FS calls in the port's sources:"
	sed 's/^/    /' <<<"$hits"
	fail=1
fi
# every absolute SD path the port writes must be under the sandbox
# (a line marked "nx2: read-only path" only proposes a folder to read from)
if hits=$(grep -nE '"sdmc:/' $src_files | grep -vE '"sdmc:/"|"sdmc:/switch/uzdoom-nx2|nx2: read-only path' ); then
	echo "NAND-SAFETY FAIL: SD card paths outside sdmc:/switch/uzdoom-nx2 in the port's sources:"
	sed 's/^/    /' <<<"$hits"
	fail=1
fi

if [ $fail = 0 ]; then
	echo "ok: no NAND/system write entry point linked, SD firewall linked, sources clean ($(wc -l <<<"$syms") symbols checked)"
fi
exit $fail
