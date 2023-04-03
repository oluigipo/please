#include "build.h"

enum Build_Os
{
	Build_Os_Null,
	Build_Os_Windows,
	Build_Os_Linux,
	Build_Os_Win32OnLinux,
	Build_Os_Sdl2OnLinux,
};

struct
{
	Cstr name;
	Cstr def;
	Cstr obj;
	Cstr exe; // NOTE(ljre): String already has '.' prefix (e.g. ".exe")
	Cstr dll;
}
static const g_osinfo[] = {
	[Build_Os_Null] = { "unknown" },
	[Build_Os_Windows] = { "windows", "WIN32", "obj", ".exe", "dll" },
	[Build_Os_Linux] = { "linux", "LINUX", "o", "", "so" },
	[Build_Os_Win32OnLinux] = { "win32linux", "WIN32LINUX", "obj", ".exe", "dll" },
	[Build_Os_Sdl2OnLinux] = { "sdl2linux", "SDL2LINUX", "o", "", "so" },
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
#   if !defined(_WIN64)
	.m32 = true,
#   endif
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
static Cstr f_optimize[3] = { "-O0", "-O1", "-O2 -static -fno-strict-aliasing", };
static Cstr f_warnings =
"-Wall -Wno-unused-function -Werror-implicit-function-declaration -Wno-logical-op-parentheses "
"-Wno-missing-braces -Wconversion -Wno-sign-conversion -Wno-implicit-int-float-conversion -Wsizeof-array-decay "
"-Wno-assume -Wno-unused-command-line-argument -Wno-int-to-void-pointer-cast -Wno-void-pointer-to-int-cast "
"";
static Cstr f_debuginfo = "-g";
static Cstr f_define = "-D";
static Cstr f_verbose = "-v";
static Cstr f_output = "-o";
static Cstr f_output_obj = "-c -o";
static Cstr f_analyze = "--analyze";
static Cstr f_incfile = "-include";
static Cstr f_m32 = "-msse2";
static Cstr f_m64 = "-march=x86-64";
static Cstr f_lto = "-flto";
static Cstr f_dll = "-Wl,/NOENTRY,/DLL -lkernel32 -llibvcruntime -lucrt";

static Cstr f_ldflags_graphic = "-Wl,/subsystem:windows";

static Cstr f_rc = "llvm-rc tools/windows-resource-file.rc /FO build/windows-resource-file.res";

#elif defined(_MSC_VER)
static Cstr f_cc = "cl /nologo /std:c11";
static Cstr f_cxx = "cl /nologo /std:c++14 /GR- /EHa-";
static Cstr f_cflags = "/Isrc /Iinclude /MT";
static Cstr f_ldflags = "/link /incremental:no";
static Cstr f_optimize[3] = { "/Og", "/Os", "/O2" };
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
static Cstr f_dll = "/NOENTRY /DLL kernel32.lib libvcruntime.lib libucrt.lib";

static Cstr f_ldflags_graphic = "/subsystem:windows";

static Cstr f_rc = "rc tools\\windows-resource-file.rc /FO build\\windows-resource-file.res";

#elif defined(__GNUC__)
static Cstr f_cc = "gcc -std=c11";
static Cstr f_cxx = "g++ -std=c++11 -fno-exceptions -fno-rtti";
static Cstr f_cflags = "-Isrc -Iinclude";
static Cstr f_ldflags = "-lntdll";
static Cstr f_optimize[3] = { "-O0", "-O1", "-O2 -fno-strict-aliasing" };
static Cstr f_warnings = "-Wall";
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
static Cstr f_dll = "-r -lkernel32 -llibvcruntime -lucrt";

static Cstr f_ldflags_graphic = "-mwindows";
static Cstr f_rc = "windres tools/windows-resource-file.rc -o build/windows-resource-file.res";

#endif

static Cstr
GenTargetString(void)
{
	Cstr arch = g_opts.m32 ? "i686" : "x86_64";
	Cstr os = g_osinfo[g_opts.os].name;
	
	int amount = snprintf(NULL, 0, "%s-%s", arch, os);
	char* str = malloc(amount+1);
	snprintf(str, amount+1, "%s-%s", arch, os);
	
	return str;
}

static Cstr
GenTargetArg(void)
{
#ifdef __clang__
	static char* str = NULL;
	
	if (!str)
	{
		Cstr arch = g_opts.m32 ? "i686" : "x86_64";
		Cstr os = NULL;
		
		switch (g_opts.os)
		{
			default: os = "unknown"; break;
			case Build_Os_Windows: os = "pc-windows-msvc"; break;
			case Build_Os_Linux: os = "linux-gnu"; break;
			case Build_Os_Win32OnLinux: os = "pc-windows-msvc"; break;
			case Build_Os_Sdl2OnLinux: os = "linux-gnu"; break;
		}
		
		int amount = snprintf(NULL, 0, "--target=%s-%s %s", arch, os, g_opts.m32 ? f_m32 : f_m64);
		str = malloc(amount+1);
		snprintf(str, amount+1, "--target=%s-%s %s", arch, os, g_opts.m32 ? f_m32 : f_m64);
	}
	
	return str;
#else
	return g_opts.m32 ? f_m32 : f_m64;
#endif
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
			snprintf(path, sizeof(path), "include/%s_ps.inc", shader->output);
			
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
	bool ok = true;
	
	Append(&head, end, "fxc /nologo /O3 src/%s /Fhinclude/%s_vs.inc", shader->path, shader->output);
	Append(&head, end, " /Tvs_%s /E%s /DPROFILE_%s", shader->profile, shader->vertex, shader->profile);
	if (shader->varprefix)
		Append(&head, end, " /Vn %s%s_vs", shader->varprefix, shader->output);
	ok = ok && RunCommand(cmd) == 0;
	
	head = cmd;
	Append(&head, end, "fxc /nologo /O3 src/%s /Fhinclude/%s_ps.inc", shader->path, shader->output);
	Append(&head, end, " /Tps_%s /E%s /DPROFILE_%s", shader->profile, shader->pixel, shader->profile);
	if (shader->varprefix)
		Append(&head, end, " /Vn %s%s_ps", shader->varprefix, shader->output);
	ok = ok && RunCommand(cmd) == 0;
	
	return ok;
}

static bool
CompileTu(struct Build_Tu* tu, Cstr* defines)
{
	char cmd[4096] = { 0 };
	char* head = cmd;
	char* end = cmd+sizeof(cmd);
	
	if (!g_opts.force_rebuild && !NeedsRebuild(tu, g_osinfo[g_opts.os].obj))
		return true;
	
	Append(&head, end, "%s src/%s", tu->is_cpp ? f_cxx : f_cc, tu->path);
	Append(&head, end, " %s %s", f_cflags, f_warnings);
	Append(&head, end, " %s", GenTargetArg());
	Append(&head, end, " %sCONFIG_OS_%s", f_define, g_osinfo[g_opts.os].def);
	Append(&head, end, " %sCONFIG_M%s", f_define, g_opts.m32 ? "32" : "64");
	
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
	
	for (Cstr* it = defines; it && *it; ++it)
		Append(&head, end, " %s%s", f_define, *it);
	for (Cstr* argv = g_opts.extra_flags; argv && *argv; ++argv)
		Append(&head, end, " \"%s\"", *argv);
	
#if defined(__clang__) || defined(__GNUC__)
	Append(&head, end, " -MMD -MF build/%s-%s.%s.d", g_target, tu->name, g_osinfo[g_opts.os].obj);
#endif
	
	Append(&head, end, " %sbuild/%s-%s.%s", f_output_obj, g_target, tu->name, g_osinfo[g_opts.os].obj);
	
#if defined(_MSC_VER) && !defined(__clang__)
	Append(&head, end, " /showIncludes | \"%s\" --redirect-msvc-show-includes build/%s-%s.%s.d src/%s", g_self, g_target, tu->name, g_osinfo[g_opts.os].obj, tu->path);
#endif
	
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
		if (!CompileTu(*tu, exec->defines))
			return false;
	}
	
	char cmd[4096] = { 0 };
	char* head = cmd;
	char* end = cmd+sizeof(cmd);
	
	Append(&head, end, "%s %sbuild/%s-%s", f_cc, f_output, g_target, exec->outname);
	if (exec->is_dll)
		Append(&head, end, ".%s", g_osinfo[g_opts.os].dll);
	else
		Append(&head, end, "%s", g_osinfo[g_opts.os].exe);
	
	for (struct Build_Tu** tu = exec->tus; *tu; ++tu)
		Append(&head, end, " build/%s-%s.%s", g_target, (*tu)->name, g_osinfo[g_opts.os].obj);
	Append(&head, end, " %s", GenTargetArg());
	
	if (g_opts.do_rc && !exec->is_dll)
		Append(&head, end, " build/windows-resource-file.res");
	if (g_opts.debug_info)
		Append(&head, end, " %s", f_debuginfo);
	if (g_opts.verbose)
		Append(&head, end, " %s", f_verbose);
	if (g_opts.lto)
		Append(&head, end, " %s %s", f_lto, f_optimize[g_opts.optimize]);
	if (g_opts.tracy)
		Append(&head, end, " TracyClient.%s", g_osinfo[g_opts.os].obj);
	if (g_opts.asan)
		Append(&head, end, " -fsanitize=address");
	if (g_opts.ubsan)
		Append(&head, end, " -fsanitize=undefined -fno-sanitize=alignment");
	
	Append(&head, end, " %s", f_ldflags);
	
	if (exec->is_dll)
		Append(&head, end, " %s", f_dll);
	if (exec->is_graphic_program && (g_opts.os == Build_Os_Windows || g_opts.os == Build_Os_Win32OnLinux))
		Append(&head, end, " %s", f_ldflags_graphic);
	
	for (Cstr* argv = g_opts.extra_flags; argv && *argv; ++argv)
		Append(&head, end, " \"%s\"", *argv);
	
	return RunCommand(cmd) == 0;
}

static bool
CompileWindowsResourceFile(void)
{
	if (!FileNeedsRebuild("tools/windows-resource-file.rc", "build/windows-resource-file.res"))
		return true;
	
	char cmd[4096] = { 0 };
	char* head = cmd;
	char* end = cmd+sizeof(cmd);
	
	Append(&head, end, "%s", f_rc);
	
	return RunCommand(cmd) == 0;
}

static bool
ValidateFlags(void)
{
	bool result = false;
	
	if (g_opts.do_rc && g_opts.os != Build_Os_Windows && g_opts.os != Build_Os_Win32OnLinux)
		fprintf(stderr, "[error] cannot use '-rc' if not building for windows.\n");
	else
		result = true;
	
	return result;
}

static int RedirectMsvcShowIncludes(int argc, char** argv);
static int PrintHelp(void);

int
main(int argc, char** argv)
{
	g_self = argv[0];
	
	if (argc >= 2 && strcmp(argv[1], "--redirect-msvc-show-includes") == 0)
		return RedirectMsvcShowIncludes(argc, argv);
	
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "/?") || !strcmp(argv[i], "-help") || !strcmp(argv[i], "--help"))
			return PrintHelp();
		else if (strcmp(argv[i], "-g") == 0)
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
		else if (strcmp(argv[i], "-profile=steam") == 0)
		{
			g_opts.lto = true;
			g_opts.steam = true;
			g_opts.do_rc = true;
			g_opts.optimize = 2;
			g_opts.debug_mode = false;
#if !defined(_MSC_VER) || defined(__clang__)
			g_opts.embed = true;
#endif
		}
		else if (strcmp(argv[i], "-profile=release") == 0)
		{
			g_opts.lto = true;
			g_opts.do_rc = true;
			g_opts.optimize = 2;
			g_opts.debug_mode = false;
		}
		else if (strncmp(argv[i], "-os=", 4) == 0)
		{
			Cstr osname = argv[i] + 4;
			int osnamelen = strlen(osname);
			enum Build_Os os = 0;
			
			for (int i = 1; i < ArrayLength(g_osinfo); ++i)
			{
				if (strncmp(osname, g_osinfo[i].name, osnamelen) == 0)
				{
					os = i;
					break;
				}
			}
			
			if (!os)
				fprintf(stderr, "[warning] unknown OS '%s'.\n", osname);
			else
			{
				g_opts.os = os;
			}
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
	ok = ok && CreateDirIfNeeded(g_builddir);
	if (g_opts.do_rc)
		ok = ok && CompileWindowsResourceFile();
	ok = ok && CompileExecutable(g_opts.exec);
	
	return !ok;
}

static int
RedirectMsvcShowIncludes(int argc, char** argv)
{
	if (argc < 4)
		return 1;
	
	FILE* output = fopen(argv[2], "wb");
	if (!output)
		return 1;
	
	fputs(argv[3], output);
	fputc('\n', output);
	
	char line[4096];
	int linecount = 0;
	
	while (fgets(line, sizeof(line), stdin))
	{
		const char prefix[] = "Note: including file:";
		
		if (strncmp(line, prefix, sizeof(prefix)-1) != 0)
			fputs(line, stdout);
		else
		{
			char* substr = line + sizeof(prefix)-1;
			while (substr[0] == ' ')
				++substr;
			
			if (!strstr(substr, "Windows Kits") && !strstr(substr, "\\VC\\Tools\\MSVC\\"))
				fputs(substr, output);
		}
	}
	
	fclose(output);
	return 0;
}

static int
PrintHelp(void)
{
	g_target = GenTargetString();
	
	fprintf(stdout,
		"usage: %s PROJECT_NAME [OPTIONS...]\n"
		"default project: %s\n"
		"\n"
		"C Compiler:    %s\n"
		"C++ Compiler:  %s\n"
		"Target:        %s\n"
		"\n"
		"OPTIONS:\n"
		"    -g                  Generate debug info (e.g. PDB files)\n"
		"    -ndebug             Don't define CONFIG_DEBUG macro\n"
		"    -asan               Use address sanitizer\n"
		"    -ubsan              Use undefined behaviour sanitizer\n"
		"    -rc                 Generate and link .res file on windows target\n"
		"    -m32                Target i686 code (32-bit mode), so ARCH becomes i686 instead of x86_64\n"
		"    -v                  Verbose output (clang and gcc: pass -v flag)\n"
		"    -tracy              Use TracyC++ instrumentation (requires TracyClient.obj and Clang)\n"
		"    -steam              Enable Steam API (requires include/steam/ and lib/steam_api[64].lib)\n"
		"    -rebuild            Force rebuild\n"
		"    -lto                Enable link-time optimizations\n"
		"    -embed              Embed assets in executable file (doesn't work on MSVC)\n"
		"    -analyze            Run static analyzer instead of compiling (requires Clang)\n"
		"    -O0 -O1 -O2         Optimization flags\n"
		"    -profile=steam      alias for: -lto -rc -O2 -ndebug -embed -steam\n"
		"    -profile=release    alias for: -lto -rc -O2 -ndebug\n"
		"",
		g_self, g_opts.exec->name, f_cc, f_cxx, g_target);
	
	for (int i = 1; i < ArrayLength(g_osinfo); ++i)
	{
		fprintf(stdout,
			"    -os=%s   \tDefines CONFIG_OS_%s\n"
			"", g_osinfo[i].name, g_osinfo[i].def);
	}
	
	return 0;
}
