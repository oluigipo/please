#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define ArrayLength(x) (sizeof(x)/sizeof*(x))

typedef const char* Cstr;

struct Build_Shader
{
	Cstr name;
	Cstr output;
	Cstr profile;
	Cstr entry_point;
};

struct Build_Project
{
	Cstr name;
	Cstr outname;
	bool is_executable;
	bool is_graphic_program;
	Cstr* deps;
	struct Build_Shader* shaders;
}
static g_projects[] = {
	{
		.name = "game_test",
		.outname = "game",
		.is_executable = true,
		.is_graphic_program = true,
		.deps = (Cstr[]) { "engine", NULL },
		.shaders = (struct Build_Shader[]) {
			{ "shader_scene3d.hlsl", "game_test_scene3d_vs.inc", "vs_4_0_level_9_3", "scene3d_d3d_vs" },
			{ "shader_scene3d.hlsl", "game_test_scene3d_ps.inc", "ps_4_0_level_9_3", "scene3d_d3d_ps" },
			{ NULL },
		},
	},
	{
		.name = "engine",
		.outname = "engine",
		.deps = (Cstr[]) { "os", NULL },
		.shaders = (struct Build_Shader[]) {
			{ "shader_quad.hlsl", "d3d11_vshader_quad.inc", "vs_4_0_level_9_3", "D3d11Shader_QuadVertex" },
			{ "shader_quad.hlsl", "d3d11_pshader_quad.inc", "ps_4_0_level_9_3", "D3d11Shader_QuadPixel" },
			{ NULL },
		},
	},
	{
		.name = "gamepad_db_gen",
		.outname = "gamepad_db_gen",
		.is_executable = true,
		.deps = (Cstr[]) { NULL },
	},
};

struct
{
	struct Build_Project* project;
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
	Cstr* extra_flags;
}
static g_opts = {
	.project = &g_projects[0],
	.debug_mode = true,
};

#ifdef __clang__
static Cstr f_cc = "clang";
static Cstr f_cflags = "-std=c11 -Isrc -Iinclude";
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

static Cstr f_ldflags_graphic = "-Wl,/subsystem:windows";

static Cstr f_hotcflags = "-DCONFIG_ENABLE_HOT -Wno-dll-attribute-on-redeclaration";
static Cstr f_hotldflags = "-Wl,/NOENTRY,/DLL -lkernel32 -llibvcruntime -lucrt";

static Cstr f_rc = "llvm-rc";

#elif defined(_MSC_VER)
static Cstr f_cc = "cl /nologo";
static Cstr f_cflags = "/std:c11 /Isrc /Iinclude";
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

static Cstr f_ldflags_graphic = "/subsystem:windows";

static Cstr f_hotcflags = "/DCONFIG_ENABLE_HOT";
static Cstr f_hotldflags = "/NOENTRY /DLL kernel32.lib libvcruntime.lib libucrt.lib";

static Cstr f_rc = "rc";

#elif defined(__GNUC__)
static Cstr f_cc = "gcc";
static Cstr f_cflags = "-std=c11 -Isrc -march=x86-64-v2 -static";
static Cstr f_ldflags = "-mwindows";
static Cstr f_optimize[3] = { "-O0", "-O1", "-O2 -ffast-math" };
static Cstr f_warnings = "-w";
static Cstr f_debuginfo = "-g";
static Cstr f_define = "-D";
static Cstr f_verbose = "-v";
static Cstr f_output = "-o";
static Cstr f_analyze = "";
static Cstr f_incfile = "-include";
#error TODO

#endif

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

static int
RunCommand(Cstr cmd)
{
	fprintf(stderr, "[cmd] %s\n", cmd);
	return system(cmd);
}

static struct Build_Project*
FindProject(Cstr name)
{
	struct Build_Project* result = NULL;
	
	for(int i = 0; i < ArrayLength(g_projects); ++i)
	{
		if (strcmp(name, g_projects[i].name) == 0)
		{
			result = &g_projects[i];
			break;
		}
	}
	
	if (!result)
		fprintf(stderr, "[warning] unknown project '%s'\n", name);
	
	return result;
}

static int
Build(struct Build_Project* project)
{
	char cmd[4096] = { 0 };
	char* end = cmd+sizeof(cmd);
	char* head = cmd;
	
	if (project->shaders)
	{
		for (struct Build_Shader* it = project->shaders; it->name; ++it)
		{
			Append(&head, end, "fxc /nologo src/%s%c%s", project->name, (project->is_executable ? '/' : '_'), it->name);
			Append(&head, end, " /Fhinclude/%s", it->output);
			Append(&head, end, " /T%s /E%s", it->profile, it->entry_point);
			
			int result = RunCommand(cmd);
			if (result)
				return result;
			
			head = cmd;
		}
	}
	
	Append(&head, end, "%s", f_cc);
	
	if (project->is_executable)
		Append(&head, end, " src/%s/unity_build.c", project->name);
	else if (g_opts.hot)
	{
		Append(&head, end, " src/%s.c", project->name);
		
		for (Cstr* it = project->deps; *it; ++it)
			Append(&head, end, " src/%s.c", *it);
	}
	else
	{
		// NOTE(ljre): Hack with '-include' flag... won't work with MSVC :kekw
		Cstr previous = project->name;
		
		for (Cstr* it = project->deps; *it; ++it)
		{
			Append(&head, end, " %s %s.c", f_incfile, previous);
			previous = *it;
		}
		
		Append(&head, end, " src/%s.c", previous);
	}
	
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
		Append(&head, end, " %sTRACY_ENABLE TracyClient.obj", f_define);
	if (g_opts.m32)
		Append(&head, end, " %s", f_m32);
	else
		Append(&head, end, " %s", f_m64);
	
	for (Cstr* argv = g_opts.extra_flags; argv && *argv; ++argv)
		Append(&head, end, " \"%s\"", *argv);
	
	if (g_opts.hot)
	{
		Append(&head, end, " %sbuild/hot-%s.dll", f_output, project->outname);
		Append(&head, end, " %s %s %s", f_hotcflags, f_ldflags, f_hotldflags);
		
		if (project->is_executable)
		{
			for (Cstr* it = project->deps; *it; ++it)
			{
				struct Build_Project* dep = FindProject(*it);
				
				Append(&head, end, " build/hot-%s.lib", dep->outname);
			}
		}
	}
	else
	{
		if (project->is_executable)
			Append(&head, end, " %sbuild/%s.exe", f_output, project->outname);
		else
			Append(&head, end, " %sbuild/%s.obj", f_output_obj, project->outname);
		
		Append(&head, end, " %s", f_ldflags);
		if (project->is_graphic_program)
			Append(&head, end, " %s", f_ldflags_graphic);
		
		if (project->is_executable)
		{
			for (Cstr* it = project->deps; *it; ++it)
			{
				struct Build_Project* dep = FindProject(*it);
				
				if (dep->is_executable)
					Append(&head, end, " build/%s.lib", dep->outname);
				else
					Append(&head, end, " build/%s.obj", dep->outname);
			}
			
			if (g_opts.do_rc)
				Append(&head, end, " build/windows-resource-file.res");
		}
	}
	
	return RunCommand(cmd);
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
		else
		{
			struct Build_Project* project = FindProject(argv[i]);
			
			if (project)
				g_opts.project = project;
		}
	}
	
	bool ok = true;
	
	ok = ok && !RunCommand("cmd /c if not exist build mkdir build");
	if (g_opts.do_rc)
		ok = ok && !RunCommand("llvm-rc windows-resource-file.rc /FO build\\windows-resource-file.res");
	
	for (Cstr* it = g_opts.project->deps; *it; ++it)
		ok = ok && !Build(FindProject(*it));
	
	ok = ok && !Build(g_opts.project);
	
	if (g_opts.hot)
	{
		ok = ok && !RunCommand("cmd /c echo ; > \"build/empty.c\"");
		ok = ok && !RunCommand("clang -fuse-ld=lld -g build/empty.c build/hot-engine.lib -o build/game.exe");
	}
	
	return !ok;
}
