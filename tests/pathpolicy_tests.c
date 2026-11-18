/*
** Host tests of the SD card write policy (engine/.../nx/nx2_pathpolicy.c).
**   cc -I engine/src/common/platform/posix/nx tests/pathpolicy_tests.c \
**      engine/src/common/platform/posix/nx/nx2_pathpolicy.c -o pathpolicy_tests
** Run by scripts/test.sh.
*/
#include <stdio.h>
#include <string.h>
#include "nx2_pathpolicy.h"

static int g_fail = 0, g_count = 0;

static void Expect(const char *cwd, const char *path, NX2PathOp op, NX2PathVerdict want)
{
	char canon[NX2_PATH_MAX + 1];
	NX2PathVerdict got = NX2_PathCheck(cwd, path, op, canon);
	g_count++;
	if (got != want)
	{
		g_fail++;
		printf("FAIL cwd=%s path=%s op=%d: got '%s' (canon %s), want '%s'\n", cwd ? cwd : "(null)", path ? path : "(null)", (int)op,
			NX2_PathVerdictName(got), canon, NX2_PathVerdictName(want));
	}
}

static void ExpectCanon(const char *cwd, const char *path, const char *want)
{
	char canon[NX2_PATH_MAX + 1];
	NX2PathVerdict v = NX2_PathCanonical(cwd, path, canon, sizeof(canon));
	g_count++;
	if (v != NX2_PATH_OK || strcmp(canon, want) != 0)
	{
		g_fail++;
		printf("FAIL canon cwd=%s path=%s: got '%s' (%s), want '%s'\n", cwd, path, canon, NX2_PathVerdictName(v), want);
	}
}

int main(void)
{
	const char *root = "/switch/uzdoom-nx2/";
	const NX2PathOp W = NX2_OP_WRITE_FILE, MK = NX2_OP_MAKE_DIR, RD = NX2_OP_REMOVE_DIR, CD = NX2_OP_CHDIR;

	// canonical form
	ExpectCanon(root, "logs/boot.log", "/switch/uzdoom-nx2/logs/boot.log");
	ExpectCanon(root, "./logs//./boot.log", "/switch/uzdoom-nx2/logs/boot.log");
	ExpectCanon("/", "sdmc:/switch/uzdoom-nx2/a", "/switch/uzdoom-nx2/a");
	ExpectCanon(root, "sdmc:x", "/switch/uzdoom-nx2/x");
	ExpectCanon(root, "a/b/../c", "/switch/uzdoom-nx2/a/c");
	ExpectCanon(root, "/", "/");

	// allowed: inside the sandbox
	Expect(root, "logs/engine.log", W, NX2_PATH_OK);
	Expect(root, "sdmc:/switch/uzdoom-nx2/saves/p1/save0.zds", W, NX2_PATH_OK);
	Expect("/", "/switch/uzdoom-nx2/config/uzdoom.ini", W, NX2_PATH_OK);
	Expect("/", "/SWITCH/UZDoom-NX2/x.txt", W, NX2_PATH_OK);
	Expect(root, "cache/uzdoom/../uzdoom/vk.bin", W, NX2_PATH_OK);
	Expect(root, "logs/anterior-5", RD, NX2_PATH_OK);
	Expect("/", "sdmc:/switch/uzdoom-nx2", MK, NX2_PATH_OK);
	Expect("/", "sdmc:/switch/uzdoom-nx2/", CD, NX2_PATH_OK);
	Expect(root, "mods/Café é.pk3", W, NX2_PATH_OK);

	// the sandbox directory itself is never written, removed or renamed
	Expect("/", "sdmc:/switch/uzdoom-nx2", W, NX2_PATH_IS_ROOT);
	Expect("/", "sdmc:/switch/uzdoom-nx2/", RD, NX2_PATH_IS_ROOT);
	Expect(root, ".", RD, NX2_PATH_IS_ROOT);
	Expect(root, "logs/..", W, NX2_PATH_IS_ROOT);

	// outside
	Expect("/", "sdmc:/switch/rdoom/x", W, NX2_PATH_OUTSIDE);
	Expect("/", "sdmc:/switch", MK, NX2_PATH_OUTSIDE);
	Expect("/", "sdmc:/switch/uzdoom-nx2x/a", W, NX2_PATH_OUTSIDE);
	Expect("/", "sdmc:/switch/uzdoom-nx", W, NX2_PATH_OUTSIDE);
	Expect("/", "sdmc:/.mesa/mesa_shader_cache_sf", RD, NX2_PATH_OUTSIDE);
	Expect("/", "/Nintendo/Contents/registered", RD, NX2_PATH_OUTSIDE);
	Expect("/", "/atmosphere/config/system_settings.ini", W, NX2_PATH_OUTSIDE);
	Expect("/", "/bootloader/hekate_ipl.ini", W, NX2_PATH_OUTSIDE);
	Expect(root, "../x", W, NX2_PATH_OUTSIDE);
	Expect(root, "../../Nintendo/a", W, NX2_PATH_OUTSIDE);
	Expect(root, "logs/../../../Nintendo", RD, NX2_PATH_OUTSIDE);
	Expect(root, "/x.txt", W, NX2_PATH_OUTSIDE);
	Expect("/switch/", "uzdoom-nx2-evil/a", W, NX2_PATH_OUTSIDE);
	Expect("/", "", W, NX2_PATH_EMPTY);
	Expect("/", NULL, W, NX2_PATH_EMPTY);

	// escapes, devices, characters, odd names
	Expect(root, "../../../x", W, NX2_PATH_ESCAPES_ROOT);
	Expect("/", "/../switch/uzdoom-nx2/a", W, NX2_PATH_ESCAPES_ROOT);
	Expect("/", "bis:/system/x", W, NX2_PATH_BAD_DEVICE);
	Expect("/", "user:/x", W, NX2_PATH_BAD_DEVICE);
	Expect("/", "save:/x", W, NX2_PATH_BAD_DEVICE);
	Expect("/", "SDMC:/switch/uzdoom-nx2/a", W, NX2_PATH_BAD_DEVICE);
	Expect("/", "sdmc:/switch/uzdoom-nx2/a:b", W, NX2_PATH_BAD_DEVICE);
	Expect("/", "sdmc:/switch/uzdoom-nx2\\..\\..\\x", W, NX2_PATH_BAD_CHAR);
	Expect("/", "sdmc:/switch/uzdoom-nx2/a\nb", W, NX2_PATH_BAD_CHAR);
	Expect("/", "sdmc:/switch/uzdoom-nx2/*", W, NX2_PATH_BAD_CHAR);
	Expect("/", "sdmc:/switch/uzdoom-nx2/.../x", W, NX2_PATH_BAD_COMPONENT);
	Expect("/", "sdmc:/switch/uzdoom-nx2/.. /x", W, NX2_PATH_BAD_COMPONENT);
	Expect("/", "sdmc:/switch./uzdoom-nx2/x", W, NX2_PATH_BAD_COMPONENT);
	Expect("/", "sdmc:/switch/uzdoom-nx2 /x", W, NX2_PATH_BAD_COMPONENT);
	Expect("/", "sdmc:/switch/uzdoom-nx2/x.", W, NX2_PATH_BAD_COMPONENT);

	// working directory must be known and absolute for relative paths
	Expect(NULL, "logs/x", W, NX2_PATH_BAD_CWD);
	Expect("", "logs/x", W, NX2_PATH_BAD_CWD);
	Expect("relative", "logs/x", W, NX2_PATH_BAD_CWD);
	Expect(NULL, "/switch/uzdoom-nx2/x", W, NX2_PATH_OK);

	// length
	{
		char longp[NX2_PATH_MAX + 64];
		strcpy(longp, "/switch/uzdoom-nx2/");
		while (strlen(longp) < NX2_PATH_MAX + 10)
			strcat(longp, "a");
		Expect("/", longp, W, NX2_PATH_TOO_LONG);
	}

	printf("pathpolicy: %d/%d passed\n", g_count - g_fail, g_count);
	return g_fail ? 1 : 0;
}
