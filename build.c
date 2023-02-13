#ifdef _WIN32
#   define _CRT_SECURE_NO_WARNINGS
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#define ArrayLength(x) (sizeof(x)/sizeof*(x))

typedef const char* Cstr;

enum Build_Os
{
	Build_Os_Null,
	Build_Os_Windows,
	Build_Os_Linux,
};

struct
{
	Cstr name;
	Cstr obj;
	Cstr exe;
}
static const g_osinfo[] = {
	[Build_Os_Null] = { "unknown" },
	[Build_Os_Windows] = { "windows", "obj", "exe" },
	[Build_Os_Linux] = { "linux", "o", "" },
};

struct Build_Tu
{
	Cstr name;
	Cstr path;
	bool is_cpp;
};

struct Build_Shader
{
	Cstr path;
	Cstr output;
	Cstr profile;
	Cstr entry_point;
};

struct Build_Executable
{
	Cstr name;
	Cstr outname;
	bool is_graphic_program;
	struct Build_Tu** tus;
	struct Build_Shader* shaders;
};

static struct Build_Tu tu_os = { "os", "os.c" };
static struct Build_Tu tu_engine = { "engine", "engine.c" };
static struct Build_Tu tu_game_test = { "game_test", "game_test/game.c" };
static struct Build_Tu tu_steam = { "steam", "steam.cpp", .is_cpp = true };
static struct Build_Tu tu_gamepad_db_gen = { "gamepad_db_gen", "gamepad_db_gen/main.c" };

static struct Build_Executable g_executables[] = {
	{
		.name = "game_test",
		.outname = "game",
		.is_graphic_program = true,
		.tus = (struct Build_Tu*[]) { &tu_engine, &tu_game_test, &tu_os, &tu_steam, NULL },
		.shaders = (struct Build_Shader[]) {
			{ "game_test/shader_scene3d.hlsl", "game_test_scene3d_vs.inc", "vs_4_0_level_9_3", "scene3d_d3d_vs" },
			{ "game_test/shader_scene3d.hlsl", "game_test_scene3d_ps.inc", "ps_4_0_level_9_3", "scene3d_d3d_ps" },
			{ "engine_shader_quad.hlsl", "d3d11_vshader_quad.inc", "vs_4_0_level_9_3", "D3d11Shader_QuadVertex" },
			{ "engine_shader_quad.hlsl", "d3d11_pshader_quad.inc", "ps_4_0_level_9_3", "D3d11Shader_QuadPixel" },
			{ NULL },
		},
	},
	{
		.name = "gamepad_db_gen",
		.outname = "gamepad_db_gen",
		.tus = (struct Build_Tu*[]) { &tu_gamepad_db_gen, NULL },
	},
};

struct
{
	struct Build_Executable* exec;
	enum Build_Os os;
	int optimize;
	bool asan;
	bool ubsan;
	bool debug_info;
	bool debug_mode;
	bool verbose;
	bool tracy;
	bool hot;
	bool force_rebuild;
	bool analyze;
	bool do_rc;
	bool m32;
	bool steam;
	bool lto;
	bool embed;
	Cstr* extra_flags;
}
static g_opts = {
#ifdef _WIN32
	.os = Build_Os_Windows,
#elif defined(__linux__)
	.os = Build_Os_Linux,
#endif
	.exec = &g_executables[0],
	.debug_mode = true,
};

#ifdef __clang__
static Cstr f_cc = "clang -std=c11";
static Cstr f_cxx = "clang++ -std=c++11 -fno-exceptions -fno-rtti";
static Cstr f_cflags = "-Isrc -Iinclude";
static Cstr f_ldflags = "-fuse-ld=lld -Wl,/incremental:no";
static Cstr f_optimize[3] = { "-O0", "-O1", "-O2 -ffast-math -static -fno-strict-aliasing", };
static Cstr f_warnings =
"-Wall -Wno-unused-function -Werror-implicit-function-declaration -Wno-logical-op-parentheses "
"-Wno-missing-braces -Wconversion -Wno-sign-conversion -Wno-implicit-int-float-conversion -Wsizeof-array-decay "
"-Wno-assume -Wno-unused-command-line-argument -Wno-int-to-void-pointer-cast -Wno-void-pointer-to-int-cast";
static Cstr f_debuginfo = "-g";
static Cstr f_define = "-D";
static Cstr f_verbose = "-v";
static Cstr f_output = "-o";
static Cstr f_output_obj = "-c -o";
static Cstr f_analyze = "--analyze";
static Cstr f_incfile = "-include";
static Cstr f_m32 = "-m32 -msse2";
static Cstr f_m64 = "-march=x86-64";
static Cstr f_lto = "-flto";

static Cstr f_ldflags_graphic = "-Wl,/subsystem:windows";

static Cstr f_hotcflags = "-DCONFIG_ENABLE_HOT -Wno-dll-attribute-on-redeclaration";
static Cstr f_hotldflags = "-Wl,/NOENTRY,/DLL -lkernel32 -llibvcruntime -lucrt";

static Cstr f_rc = "llvm-rc";

#elif defined(_MSC_VER)
static Cstr f_cc = "cl /nologo /std:c11";
static Cstr f_cxx = "cl /nologo /std:c++14 /GR- /EHa-";
static Cstr f_cflags = "/Isrc /Iinclude /MT";
static Cstr f_ldflags = "/link /incremental:no";
static Cstr f_optimize[3] = { "/Og", "/Os", "/O2 /fp:fast" };
static Cstr f_warnings = "/W3";
static Cstr f_debuginfo = "/Zi";
static Cstr f_define = "/D";
static Cstr f_verbose = "";
static Cstr f_output = "/Fe";
static Cstr f_output_obj = "/c /Fo";
static Cstr f_analyze = "";
static Cstr f_incfile = "/FI";
static Cstr f_m32 = "/arch:SSE2";
static Cstr f_m64 = "";
static Cstr f_lto = "/GL";

static Cstr f_ldflags_graphic = "/subsystem:windows";

static Cstr f_hotcflags = "/DCONFIG_ENABLE_HOT";
static Cstr f_hotldflags = "/NOENTRY /DLL kernel32.lib libvcruntime.lib libucrt.lib";

static Cstr f_rc = "rc";

#elif defined(__GNUC__)
static Cstr f_cc = "gcc -std=c11";
static Cstr f_cxx = "g++ -std=c++11 -fno-exceptions -fno-rtti";
static Cstr f_cflags = "-Isrc -march=x86-64-v2";
static Cstr f_ldflags = "";
static Cstr f_optimize[3] = { "-O0", "-O1", "-O2 -ffast-math" };
static Cstr f_warnings = "-w";
static Cstr f_debuginfo = "-g";
static Cstr f_define = "-D";
static Cstr f_verbose = "-v";
static Cstr f_output = "-o";
static Cstr f_output_obj = "-c -o";
static Cstr f_analyze = "";
static Cstr f_incfile = "-include";
static Cstr f_m32 = "-m32 -msse2";
static Cstr f_m64 = "-march=x86-64";
static Cstr f_lto = "-flto";

static Cstr f_ldflags_graphic = "-mwindows";

static Cstr f_hotcflags = "-DCONFIG_ENABLE_HOT";
static Cstr f_hotldflags = "-r -lkernel32 -llibvcruntime -lucrt";

static Cstr f_rc = "windres";

#endif

static Cstr g_target = "";

static void
Append(char** head, char* end, Cstr fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int amount = vsnprintf(*head, end-*head, fmt, args);
	va_end(args);
	
	if (*head + amount >= end)
	{
		fprintf(stderr, "[error] command buffer too small for required project!\n");
		exit(1);
	}
	
	*head += amount;
}

static Cstr
GenTargetString(void)
{
	Cstr arch = g_opts.m32 ? "i386" : "x86_64";
	Cstr os = g_osinfo[g_opts.os].name;
	
	int amount = snprintf(NULL, 0, "%s-%s", arch, os);
	char* str = malloc(amount+1);
	snprintf(str, amount+1, "%s-%s", arch, os);
	
	return str;
}

static int
RunCommand(Cstr cmd)
{
	fprintf(stderr, "[cmd] %s\n", cmd);
	return system(cmd);
}

static bool
NeedsRebuild(struct Build_Tu* tu)
{
	bool result = true;
	char path[4096];
	
	snprintf(path, sizeof(path), "build/%s-%s.%s", g_target, tu->name, g_osinfo[g_opts.os].obj);
	struct __stat64 stat_data;
	
	if (_stat64(path, &stat_data) == 0)
	{
		uint64_t objtime = stat_data.st_mtime;
		
		snprintf(path, sizeof(path), "build/%s-%s.%s.d", g_target, tu->name, g_osinfo[g_opts.os].obj);
		FILE* file = fopen(path, "rb");
		
		if (file)
		{
			fseek(file, 0, SEEK_END);
			size_t filesize = ftell(file);
			rewind(file);
			char* buf = malloc(filesize+1);
			fread(buf, 1, filesize, file);
			buf[filesize] = 0;
			fclose(file);
			
			char* const begin = buf;
			char* const end = buf + filesize;
			char* head = buf;
			
			//- Parse
			{
				// NOTE(ljre): File should begin with the output name
				const char* path_head = path;
				while (*path_head == *head)
				{
					++path_head;
					++head;
				}
				
				if (*head++ != ':')
					goto lbl_done;
				
				while (*head)
				{
					while (*head == ' ' || *head == '\t' || *head == '\r' || *head == '\n')
						++head;
					if (!*head)
						break;
					if (*head == '\\' && (head[1] == '\n' || head[1] == '\r' && head[2] == '\n'))
					{
						head += 2 + (head[1] == '\r');
						continue;
					}
					
					char depname[4096] = { 0 };
					int deplen = 0;
					while (*head && *head != ' ' && *head != '\n' && *head != '\r')
					{
						if (*head == '\\' && head[1] == ' ')
						{
							depname[deplen++] = ' ';
							head += 2;
						}
						else
							depname[deplen++] = *head++;
					}
					
					if (_stat64(depname, &stat_data) != 0)
						goto lbl_done;
					if (stat_data.st_mtime > objtime)
						goto lbl_done;
				}
				
				// Did we get here? cool, then we don't need to rebuild it!
				result = false;
			}
			
			//- Done Parsing
			lbl_done:;
			free(buf);
		}
	}
	
	return result;
}

static bool
CompileShader(struct Build_Shader* shader)
{
	// NOTE(ljre): Rebuild only if needed
	if (!g_opts.force_rebuild)
	{
		char path[4096] = { 0 };
		struct __stat64 stat_data;
		snprintf(path, sizeof(path), "src/%s", shader->path);
		
		if (_stat64(path, &stat_data) == 0)
		{
			uint64_t src_time = stat_data.st_mtime;
			snprintf(path, sizeof(path), "include/%s", shader->output);
			
			if (_stat64(path, &stat_data) == 0)
			{
				uint64_t dst_time = stat_data.st_mtime;
				
				if (src_time < dst_time)
					return true;
			}
		}
	}
	
	// NOTE(ljre): Actual build
	char cmd[4096] = { 0 };
	char* head = cmd;
	char* end = cmd+sizeof(cmd);
	
	Append(&head, end, "fxc /nologo src/%s", shader->path);
	Append(&head, end, " /Fhinclude/%s", shader->output);
	Append(&head, end, " /T%s /E%s", shader->profile, shader->entry_point);
	
	return RunCommand(cmd) == 0;
}

static bool
CompileTu(struct Build_Tu* tu)
{
	char cmd[4096] = { 0 };
	char* head = cmd;
	char* end = cmd+sizeof(cmd);
	
	if (!g_opts.force_rebuild && !NeedsRebuild(tu))
		return true;
	
	Append(&head, end, "%s src/%s", tu->is_cpp ? f_cxx : f_cc, tu->path);
	Append(&head, end, " %s %s", f_cflags, f_warnings);
	
	if (g_opts.analyze)
		Append(&head, end, " %s", f_analyze);
	if (g_opts.optimize)
		Append(&head, end, " %s", f_optimize[g_opts.optimize]);
	if (g_opts.asan)
		Append(&head, end, " -fsanitize=address");
	if (g_opts.ubsan)
		Append(&head, end, " -fsanitize=undefined -fno-sanitize=alignment");
	if (g_opts.debug_info)
		Append(&head, end, " %s", f_debuginfo);
	if (g_opts.debug_mode)
		Append(&head, end, " %s%s", f_define, "CONFIG_DEBUG");
	if (g_opts.verbose)
		Append(&head, end, " %s", f_verbose);
	if (g_opts.tracy)
		Append(&head, end, " %sTRACY_ENABLE", f_define);
	if (g_opts.steam)
		Append(&head, end, " %sCONFIG_ENABLE_STEAM", f_define);
	if (g_opts.lto)
		Append(&head, end, " %s", f_lto);
	if (g_opts.embed)
		Append(&head, end, " %sCONFIG_ENABLE_EMBED", f_define);
	if (g_opts.m32)
		Append(&head, end, " %s", f_m32);
	else
		Append(&head, end, " %s", f_m64);
	
	for (Cstr* argv = g_opts.extra_flags; argv && *argv; ++argv)
		Append(&head, end, " \"%s\"", *argv);
	
#if defined(__clang__) || defined(__GNUC__)
	Append(&head, end, " -MMD -MF build/%s-%s.%s.d", g_target, tu->name, g_osinfo[g_opts.os].obj);
#endif
	
	Append(&head, end, " %sbuild/%s-%s.%s", f_output_obj, g_target, tu->name, g_osinfo[g_opts.os].obj);
	
	return RunCommand(cmd) == 0;
}

static bool
CompileExecutable(struct Build_Executable* exec)
{
	for (struct Build_Shader* shader = exec->shaders; shader && shader->path; ++shader)
	{
		if (!CompileShader(shader))
			return false;
	}
	
	for (struct Build_Tu** tu = exec->tus; *tu; ++tu)
	{
		if (!CompileTu(*tu))
			return false;
	}
	
	char cmd[4096] = { 0 };
	char* head = cmd;
	char* end = cmd+sizeof(cmd);
	
	Append(&head, end, "%s %sbuild/%s-%s.%s", f_cc, f_output, g_target, exec->outname, g_osinfo[g_opts.os].exe);
	
	for (struct Build_Tu** tu = exec->tus; *tu; ++tu)
		Append(&head, end, " build/%s-%s.%s", g_target, (*tu)->name, g_osinfo[g_opts.os].obj);
	
	if (g_opts.do_rc)
		Append(&head, end, " build/windows-resource-file.res");
	if (g_opts.debug_info)
		Append(&head, end, " %s", f_debuginfo);
	if (g_opts.lto)
		Append(&head, end, " %s %s", f_lto, f_optimize[g_opts.optimize]);
	if (g_opts.tracy)
		Append(&head, end, " TracyClient.%s", g_osinfo[g_opts.os].obj);
	if (g_opts.m32)
		Append(&head, end, " %s", f_m32);
	else
		Append(&head, end, " %s", f_m64);
	
	Append(&head, end, " %s", f_ldflags);
	
	if (exec->is_graphic_program && g_opts.os == Build_Os_Windows)
		Append(&head, end, " %s", f_ldflags_graphic);
	
	for (Cstr* argv = g_opts.extra_flags; argv && *argv; ++argv)
		Append(&head, end, " \"%s\"", *argv);
	
	return RunCommand(cmd) == 0;
}

static bool
CreateBuildDirIfNeeded(void)
{
	bool result = false;
	
	switch (g_opts.os)
	{
		default: break;
		case Build_Os_Windows: result = !RunCommand("cmd /c if not exist build mkdir build"); break;
		case Build_Os_Linux: result = !RunCommand("if [ ! -d \"build\" ]; then mkdir build fi"); break;
	}
	
	return result;
}

static bool
ValidateFlags(void)
{
	bool result = false;
	
	if (g_opts.do_rc && g_opts.os != Build_Os_Windows)
		fprintf(stderr, "[error] cannot use '-rc' if not building for windows.\n");
	else if (g_opts.hot && g_opts.os != Build_Os_Windows)
		fprintf(stderr, "[error] hot reloading only available on windows.\n");
	else
		result = true;
	
	return result;
}

int
main(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-g") == 0)
			g_opts.debug_info = true;
		else if (strcmp(argv[i], "-ndebug") == 0)
			g_opts.debug_mode = false;
		else if (strcmp(argv[i], "-asan") == 0)
			g_opts.asan = true;
		else if (strcmp(argv[i], "-ubsan") == 0)
			g_opts.ubsan = true;
		else if (strcmp(argv[i], "-rc") == 0)
			g_opts.do_rc = true;
		else if (strcmp(argv[i], "-m32") == 0)
			g_opts.m32 = true;
		else if (strcmp(argv[i], "-v") == 0)
			g_opts.verbose = true;
		else if (strcmp(argv[i], "-tracy") == 0)
			g_opts.tracy = true;
		else if (strcmp(argv[i], "-steam") == 0)
			g_opts.steam = true;
		else if (strcmp(argv[i], "-rebuild") == 0)
			g_opts.force_rebuild = true;
		else if (strcmp(argv[i], "-lto") == 0)
			g_opts.lto = true;
		else if (strcmp(argv[i], "-embed") == 0)
		{
#if !defined(_MSC_VER) || defined(__clang__)
			g_opts.embed = true;
#else
			fprintf(stderr, "[warning] msvc does not support '.incbin'.\n");
#endif
		}
		else if (strcmp(argv[i], "--") == 0)
		{
			g_opts.extra_flags = (Cstr*)&argv[i + 1];
			break;
		}
		else if (strcmp(argv[i], "-hot") == 0)
		{
#ifdef __clang__
			g_opts.hot = true;
#else
			fprintf(stderr, "[warning] hot reloading only supported with clang for now.\n");
#endif
		}
		else if (strcmp(argv[i], "-analyze") == 0)
		{
#ifdef __clang__
			g_opts.analyze = true;
#else
			fprintf(stderr, "[warning] static analyzer only supported with clang for now.\n");
#endif
		}
		else if (strncmp(argv[i], "-O", 2) == 0)
		{
			char* end;
			int opt = strtol(argv[i]+2, &end, 10);
			
			if (end == argv[i]+strlen(argv[i]))
				g_opts.optimize = opt;
			else
				fprintf(stderr, "[warning] invalid optimization flag '%s'.\n", argv[i]);
		}
		else if (strncmp(argv[i], "-os=", 4) == 0)
		{
			Cstr osname = argv[i] + 4;
			enum Build_Os os = 0;
			
			for (int i = 1; i < ArrayLength(g_osinfo); ++i)
			{
				if (strcmp(osname, g_osinfo[i].name) == 0)
				{
					os = i;
					break;
				}
			}
			
			if (!os)
				fprintf(stderr, "[warning] unknown OS '%s'.\n", osname);
			else
				g_opts.os = os;
		}
		else if (argv[i][0] == '-')
			fprintf(stderr, "[warning] unknown flag '%s'.\n", argv[i]);
		else
		{
			struct Build_Executable* exec = NULL;
			
			for (int j = 0; j < ArrayLength(g_executables); ++j)
			{
				if (strcmp(g_executables[j].name, argv[i]) == 0)
				{
					exec = &g_executables[j];
					break;
				}
			}
			
			if (exec)
				g_opts.exec = exec;
			else
				fprintf(stderr, "[warning] unknown project '%s'.\n", argv[i]);
		}
	}
	
	g_target = GenTargetString();
	
	bool ok = true;
	
	ok = ok && ValidateFlags();
	ok = ok && CreateBuildDirIfNeeded();
	if (g_opts.do_rc)
		ok = ok && !RunCommand("llvm-rc windows-resource-file.rc /FO build\\windows-resource-file.res");
	
	ok = ok && CompileExecutable(g_opts.exec);
	
	return !ok;
}
