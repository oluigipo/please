#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ArrayLength(x) (sizeof(x)/sizeof*(x))

struct Build_Project
{
	const char* name;
	const char* outname;
	bool is_executable;
	bool is_graphic_program;
	const char** deps;
}
static g_projects[] = {
	{
		.name = "engine_test",
		.outname = "game",
		.is_executable = true,
		.is_graphic_program = true,
		.deps = (const char*[]) { "engine", NULL },
	},
	{
		.name = "engine",
		.outname = "engine",
		.is_executable = false,
		.deps = (const char*[]) { NULL },
	},
	{
		.name = "gamepad_db_gen",
		.outname = "gamepad_db_gen",
		.is_executable = true,
		.deps = (const char*[]) { NULL },
	},
};

struct
{
	struct Build_Project* project;
	int optimize;
	bool asan;
	bool debug_info;
	bool debug_mode;
	bool verbose;
	bool tracy;
	bool hot;
	bool force_rebuild;
}
static g_opts = {
	.project = &g_projects[0],
	.debug_mode = true,
};

#ifdef __clang__
static const char* f_cc = "clang";
static const char* f_cflags = "-std=c11 -Isrc -Iinclude -march=x86-64-v2";
static const char* f_ldflags = "-fuse-ld=lld -Wl,/incremental:no";
static const char* f_optimize[3] = { "-O0", "-O1", "-O2 -ffast-math -static" };
static const char* f_warnings =
"-Wall -Wno-unused-function -Werror-implicit-function-declaration -Wno-logical-op-parentheses "
"-Wno-missing-braces -Wconversion -Wno-sign-conversion -Wno-implicit-int-float-conversion -Wsizeof-array-decay "
"-Wno-assume -Wno-unused-command-line-argument";
static const char* f_debuginfo = "-g";
static const char* f_define = "-D";
static const char* f_verbose = "-v";
static const char* f_output = "-o";
static const char* f_output_obj = "-c -o";

static const char* f_ldflags_graphic = "-Wl,/subsystem:windows";

static const char* f_hotcflags = "-DINTERNAL_ENABLE_HOT -Wno-dll-attribute-on-redeclaration";
static const char* f_hotldflags = "-Wl,/NOENTRY,/DLL -lkernel32 -llibvcruntime -llibucrt";

#elif defined(_MSC_VER)
static const char* f_cc = "cl /nologo";
static const char* f_cflags = "/std:c11 /Isrc /Iinclude";
static const char* f_ldflags = "/link /incremental:no";
static const char* f_optimize[3] = { "/Og", "/Os", "/O2 /fp:fast" };
static const char* f_warnings = "/w";
static const char* f_debuginfo = "/Zi";
static const char* f_define = "/D";
static const char* f_verbose = "";
static const char* f_output = "/Fe";
static const char* f_output_obj = "/c /Fo";

static const char* f_ldflags_graphic = "/subsystem:windows";

static const char* f_hotcflags = "/DINTERNAL_ENABLE_HOT";
static const char* f_hotldflags = "/NOENTRY /DLL kernel32.lib libvcruntime.lib libucrt.lib";

#elif defined(__GNUC__)
static const char* f_cc = "gcc";
static const char* f_cflags = "-std=c11 -Isrc -march=x86-64-v2 -static";
static const char* f_ldflags = "-mwindows";
static const char* f_optimize[3] = { "-O0", "-O1", "-O2 -ffast-math" };
static const char* f_warnings = "-w";
static const char* f_debuginfo = "-g";
static const char* f_define = "-D";
static const char* f_verbose = "-v";
static const char* f_output = "-o";
#error TODO

#endif

static int
RunCommand(const char* cmd)
{
	fprintf(stderr, "[cmd] %s\n", cmd);
	return system(cmd);
}

static struct Build_Project*
FindProject(const char* name)
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
		fprintf(stderr, "warning: unknown project '%s'\n", name);
	
	return result;
}

static int
Build(struct Build_Project* project)
{
	char cmd[4096] = { 0 };
	char* end = cmd+sizeof(cmd);
	char* head = cmd;
	
	head += snprintf(head, end-head, "%s src/%s/unity_build.c", f_cc, project->name);
	head += snprintf(head, end-head, " %s %s", f_cflags, f_warnings);
	if (g_opts.optimize)
		head += snprintf(head, end-head, " %s", f_optimize[g_opts.optimize]);
	if (g_opts.asan)
		head += snprintf(head, end-head, " -fsanitize=address");
	if (g_opts.debug_info)
		head += snprintf(head, end-head, " %s", f_debuginfo);
	if (g_opts.debug_mode)
		head += snprintf(head, end-head, " %s%s", f_define, "DEBUG");
	if (g_opts.verbose)
		head += snprintf(head, end-head, " %s", f_verbose);
	if (g_opts.tracy)
		head += snprintf(head, end-head, " %sTRACY_ENABLE TracyClient.obj", f_define);
	
	if (g_opts.hot)
	{
		head += snprintf(head, end-head, " %sbuild/hot-%s.dll", f_output, project->outname);
		head += snprintf(head, end-head, " %s %s %s", f_hotcflags, f_ldflags, f_hotldflags);
		
		for (const char** it = project->deps; *it; ++it)
		{
			struct Build_Project* dep = FindProject(*it);
			
			head += snprintf(head, end-head, " build/hot-%s.lib", dep->outname);
		}
	}
	else
	{
		if (project->is_executable)
			head += snprintf(head, end-head, " %sbuild/%s.exe", f_output, project->outname);
		else
			head += snprintf(head, end-head, " %sbuild/%s.obj", f_output_obj, project->outname);
		
		head += snprintf(head, end-head, " %s", f_ldflags);
		if (project->is_graphic_program)
			head += snprintf(head, end-head, " %s", f_ldflags_graphic);
		
		for (const char** it = project->deps; *it; ++it)
		{
			struct Build_Project* dep = FindProject(*it);
			
			if (dep->is_executable)
				head += snprintf(head, end-head, " build/%s.lib", dep->outname);
			else
				head += snprintf(head, end-head, " build/%s.obj", dep->outname);
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
		else if (strcmp(argv[i], "-v") == 0)
			g_opts.verbose = true;
		else if (strcmp(argv[i], "-tracy") == 0)
			g_opts.tracy = true;
		else if (strcmp(argv[i], "-hot") == 0)
			g_opts.hot = true;
		else if (strncmp(argv[i], "-O", 2) == 0)
		{
			char* end;
			int opt = strtol(argv[i]+2, &end, 10);
			
			if (end == argv[i]+strlen(argv[i]))
				g_opts.optimize = opt;
		}
		else
		{
			struct Build_Project* project = FindProject(argv[i]);
			
			if (project)
				g_opts.project = project;
		}
	}
	
	bool ok = true;
	for (const char** it = g_opts.project->deps; *it; ++it)
		ok = ok && !Build(FindProject(*it));
	
	ok = ok && !Build(g_opts.project);
	
	if (g_opts.hot)
	{
		ok = ok && !RunCommand("cmd /c echo ; > \"build/empty.c\"");
		ok = ok && !RunCommand("clang -fuse-ld=lld build/empty.c build/hot-engine.lib -o build/hot-game.exe");
	}
	
	return !ok;
}
