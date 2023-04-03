#ifndef BUILD_H
#define BUILD_H

#ifdef _WIN32
#   define _CRT_SECURE_NO_WARNINGS
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#define ArrayLength(x) (sizeof(x)/sizeof*(x))
#define LocalPrintf(size, ...) LocalPrintfImpl_((char[size]) { 0 }, size, __VA_ARGS__)

static char* LocalPrintfImpl_(char* buf, int size, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, size, fmt, args);
	va_end(args);
	
	return buf;
}

typedef const char* Cstr;

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
	Cstr vertex, pixel;
	Cstr profile;
	Cstr varprefix;
};

struct Build_Executable
{
	Cstr name;
	Cstr outname;
	bool is_graphic_program;
	bool is_dll;
	struct Build_Tu** tus;
	struct Build_Shader* shaders;
	Cstr* defines;
};

static Cstr g_self = "";
static Cstr g_target = "";
static Cstr g_builddir = "build";

static struct Build_Tu tu_os = { "os", "os.c" };
static struct Build_Tu tu_engine = { "engine", "engine.c" };
static struct Build_Tu tu_debugtools = { "debugtools", "debugtools.c" };
static struct Build_Tu tu_game_test = { "game_test", "game_test/game.c" };
static struct Build_Tu tu_steam = { "steam", "steam.cpp", .is_cpp = true };
static struct Build_Tu tu_gamepad_db_gen = { "gamepad_db_gen", "gamepad_db_gen/main.c" };

static struct Build_Executable g_executables[] = {
	{
		.name = "game_test",
		.outname = "game",
		.is_graphic_program = true,
		.tus = (struct Build_Tu*[]) { &tu_engine, &tu_game_test, &tu_os, &tu_steam, &tu_debugtools, NULL },
		.shaders = (struct Build_Shader[]) {
			{ "engine_shader_quad.hlsl", "d3d11_shader_quad", "Vertex", "Pixel", "4_0_level_9_3", "g_render_" },
			{ "engine_shader_quad.hlsl", "d3d11_shader_quad_level91", "Vertex", "Pixel", "4_0_level_9_1", "g_render_" },
			
			{ "game_test/shader_scene3d.hlsl", "d3d11_gametest_scene3d", "Vertex", "Pixel", "4_0_level_9_3", "g_scene3d_" },
			{ "game_test/shader_scene3d.hlsl", "d3d11_gametest_scene3d_level91", "Vertex", "Pixel", "4_0_level_9_1", "g_scene3d_" },
			
			{ NULL },
		},
	},
	{
		.name = "gamepad_db_gen",
		.outname = "gamepad_db_gen",
		.tus = (struct Build_Tu*[]) { &tu_gamepad_db_gen, NULL },
	}
};

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

static bool
NeedsRebuild(struct Build_Tu* tu, const char* obj_extension)
{
	bool result = true;
	char path[4096];
	
	snprintf(path, sizeof(path), "%s/%s-%s.%s", g_builddir, g_target, tu->name, obj_extension);
	struct __stat64 stat_data;
	
	if (_stat64(path, &stat_data) == 0)
	{
		uint64_t objtime = stat_data.st_mtime;
		
		snprintf(path, sizeof(path), "%s/%s-%s.%s.d", g_builddir, g_target, tu->name, obj_extension);
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
				// NOTE(ljre): Check if file begins with output name
				const char* path_head = path;
				while (*path_head == *head)
				{
					++path_head;
					++head;
				}
				
				// NOTE(ljre): If it does and then has a ':' right after it, then we have a Makefile-like
				//             depfile.
				if (*head++ == ':')
				{
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
				}
				else
				{
					// NOTE(ljre): Otherwise, it must be MSVC's /showInclude filtered output
					head = buf;
					
					while (*head)
					{
						while (*head == '\n')
							++head;
						if (!*head)
							break;
						
						char depname[4096] = { 0 };
						int deplen = 0;
						
						while (*head && *head != '\n')
							depname[deplen++] = *head++;
						
						if (_stat64(depname, &stat_data) != 0)
							goto lbl_done;
						if (stat_data.st_mtime > objtime)
							goto lbl_done;
					}
				}
			}
			
			// Did we get here? cool, then we don't need to rebuild it!
			result = false;
			
			//- Done Parsing
			lbl_done:;
			free(buf);
		}
	}
	
	return result;
}

static bool
FileNeedsRebuild(Cstr dep, Cstr output)
{
	struct __stat64 dep_stat, output_stat;
	
	if (_stat64(dep, &dep_stat) == 0 && _stat64(output, &output_stat) == 0)
	{
		if (dep_stat.st_mtime <= output_stat.st_mtime)
			return false;
	}
	
	return true;
}

static bool
CreateDirIfNeeded(Cstr dir)
{
	bool result = false;
	
	struct _stat64 info;
	if (_stat64(dir, &info))
		result = !_mkdir(dir);
	else if (info.st_mode & _S_IFDIR)
		result = true;
	
	return result;
}

#endif //BUILD_H
