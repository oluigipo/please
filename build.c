#define _CRT_SECURE_NO_WARNINGS
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wall"
#   pragma GCC diagnostic ignored "-Wextra"
#else
#   pragma warning(push, 0)
#endif

#define NOBUILD_IMPLEMENTATION
#include "nobuild.h"

#define CMD_ASYNC(...) (\
INFO("CMD: %s", cmd_show((Cmd) { cstr_array_make(__VA_ARGS__, NULL) })),\
cmd_run_async((Cmd) { cstr_array_make(__VA_ARGS__, NULL) }, NULL, NULL)\
)
#define Length(x) (sizeof(x) / sizeof(*(x)))
#define Equals(a,b) (strcmp(a, b) == 0)

#define WINDOWS 1
#define LINUX 2

#ifdef _WIN32
#   define PLATFORM WINDOWS
#else
#   define PLATFORM LINUX
#endif

#if PLATFORM == WINDOWS
#   define LIBRARIES LIB("user32"), LIB("gdi32"), LIB("hid")
#   define TUS "engine", "game", "platform_win32"
#   define OBJ ".obj"
#   define EXE ".exe"
#   define OUTPUT_NAME "game.exe"
#elif PLATFORM == LINUX
#   define LIBRARIES LIB("m"), LIB("X11"), LIB("dl"), LIB("asound")
#   define TUS "engine", "game", "platform_linux"
#   define OBJ ".o"
#   define EXE ""
#   define OUTPUT_NAME "game"
#endif

#define BUILDDIR "build"

#ifdef __clang__
#   define CC "clang"
#   define CFLAGS "-std=gnu99", "-Iinclude", "-Wall", "-Werror-implicit-function-declaration",\
"-Wno-unused-function", "-Wconversion", "-Wno-format", "-Wno-missing-braces"
#   define OFLAGS "-O2", "-ffast-math"
#   if PLATFORM == WINDOWS
#       define LFLAGS "-fuse-ld=lld"
#   else
#       define LFLAGS "-fuse-ld=lld", "-Wl,/subsystem:windows"
#   endif
#   define OUT_EXEC(x) CONCAT("-o\"", x, EXE, "\"")
#   define OUT_OBJ(x) CONCAT("-c -o\"", x, "\"")
#   define LIB(x) CONCAT("-l", x)
#elif defined _MSC_VER
#   define CC "cl /nologo"
#   define CFLAGS "/TC /Iinclude"
#   define OFLAGS "/O2"
#   define LFLAGS "/link /subsystem:windows"
#   define OUT_EXEC(x) CONCAT("/Fe:\"", x, EXE, "\"")
#   define OUT_OBJ(x) CONCAT("/c /Fo\"", x, "\"")
#   define LIB(x) CONCAT(x, ".lib")
#elif defined __TINYC__
#   define CC "TCC"
#   define CFLAGS "-std=c99 -Iinclude"
#   define OFLAGS ""
#   define LFLAGS "-Wl,-subsystem=gui"
#   define OUT_EXEC(x) CONCAT("-o\"", x, EXE, "\"")
#   define OUT_OBJ(x) CONCAT("-c -o\"", x, "\"")
#   define LIB(x) CONCAT("-l", x)
#elif defined __GNUC__
#   define CC "gcc"
#   define CFLAGS "-std=gnu99", "-Iinclude"
#   define OFLAGS "-O2", "-ffast-math"
#   define LFLAGS "-mwindows"
#   define OUT_EXEC(x) CONCAT("-o\"", x, EXE, "\"")
#   define OUT_OBJ(x) CONCAT("-c -o\"", x, "\"")
#   define LIB(x) CONCAT("-l", x)
#endif

int main(int argc, char** argv)
{
	Cstr tus[] = { TUS };
	Pid pids[Length(tus)];
	Cstr_Array objs = { 0 };
	
	if (!IS_DIR(BUILDDIR))
		MKDIRS(BUILDDIR);
	
	for (int i = 0; i < Length(tus); ++i)
	{
		Cstr obj = PATH(BUILDDIR, CONCAT(tus[i], OBJ));
		Cstr src = PATH("src", CONCAT(tus[i], ".c"));
		
		objs = cstr_array_append(objs, obj);
		
		if (!PATH_EXISTS(obj) || is_path1_modified_after_path2(src, obj))
			pids[i] = CMD_ASYNC(CC, src, OUT_OBJ(obj), CFLAGS, OFLAGS);
		else
			pids[i] = NULL;
	}
	
	for (int i = 0; i < Length(pids); ++i)
	{
		if (pids[i])
			pid_wait(pids[i]);
	}
	
	CMD(CC, cstr_array_join(" ", objs), OUT_EXEC(PATH(BUILDDIR, "game")), LFLAGS, LIBRARIES);
	
	printf("\nCompilation done.\n");
}
