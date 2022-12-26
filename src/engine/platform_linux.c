#define _POSIX_C_SOURCE 200809L
#include "internal.h"

#define __USE_MISC
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <fcntl.h>

//~ Types

//~ Globals
static Display* global_display;
static Window global_window;

static bool32 global_window_should_close;
static int32 global_window_width;
static int32 global_window_height;
static GraphicsContext global_graphics_context;
static struct timespec global_time_begin;
static Platform_Config global_config;

//~ Functions
#include "platform_linux_input.c"
#include "platform_linux_audio.c"

#ifdef INTERNAL_ENABLE_OPENGL
#   include "platform_linux_opengl.c"
#endif

//~ Entry Point
int main(int argc, char* argv[])
{
	Trace("main - Program Entry Point");
	
	// NOTE(ljre): Basic Setup
	{
		clock_gettime(CLOCK_MONOTONIC, &global_time_begin);
	}
	
	int32 result = Engine_Main(argc, argv);
	
	if (global_display)
		XCloseDisplay(global_display);
	
	Linux_DeinitAudio();
	
	return result;
}

//~ API
API void
Platform_ExitWithErrorMessage(String message)
{
	Trace("Platform_ExitWithErrorMessage");
	
	printf("%.*s\n", message.len, message.data);
	exit(1);
	/* no return */
}

API void
Platform_MessageBox(String title, String message)
{
	Trace("Platform_MessageBox");
	
	// TODO(ljre): Real Message Box
	printf("=======================================\n");
	printf("======= %.*s\n", title.len, title.data);
	printf("=======================================\n");
	printf("%.*s\n", message.len, message.data);
}

API bool32
Platform_CreateWindow(const Platform_Config* config, const GraphicsContext** out_graphics)
{
	Trace("Platform_CreateWindow");
	char local_buffer[1 << 10];
	const char* title;
	
	if (config->window_title.len > 0 && config->window_title.data[config->window_title.len-1] == 0)
	{
		title = config->window_title.data;
	}
	else
	{
		snprintf(local_buffer, sizeof local_buffer, "%.*s", StrFmt(config->window_title));
		title = local_buffer;
	}
	
	global_display = XOpenDisplay(NULL);
	if (!global_display)
		return false;
	
	XInitThreads();
	
	bool32 ok = false;
	
	ok = ok || (config->graphics_api & GraphicsAPI_OpenGL && Linux_CreateOpenGLWindow(config->window_width, config->window_height, title));
	
	if (ok)
	{
		global_window_width = config->window_width;
		global_window_height = config->window_height;
		
		*out_graphics = &global_graphics_context;
		
		Linux_InitInput();
		Linux_InitAudio();
	}
	
	return ok;
}

API bool32
Platform_WindowShouldClose(void)
{
	return global_window_should_close;
}

API int32
Platform_WindowWidth(void)
{
	return global_window_width;
}

API int32
Platform_WindowHeight(void)
{
	return global_window_height;
}

API void
Platform_UpdateConfig(const Platform_Config* config)
{
	memcpy(&global_config, config, sizeof(global_config));
}

API float64
Platform_GetTime(void)
{
	struct timespec t;
	if (0 != clock_gettime(CLOCK_MONOTONIC, &t))
		return 0.0;
	
	t.tv_sec -= global_time_begin.tv_sec;
	t.tv_nsec -= global_time_begin.tv_nsec;
	
	if (t.tv_nsec < 0)
	{
		t.tv_nsec += 1000000000L;
		t.tv_sec -= 1;
	}
	
	return (float64)t.tv_nsec / 1000000000.0 + (float64)t.tv_sec;
}

API void
Platform_PollEvents(void)
{
	Linux_UpdateInputPre();
	// TODO(ljre): Process 'global_config'.
	
	int64 mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask;
	
	XEvent event;
	while (XCheckWindowEvent(global_display, global_window, mask, &event))
	{
		switch (event.type)
		{
			case KeyPress:
			case KeyRelease:
			{
				KeySym key = XkbKeycodeToKeysym(global_display, (KeyCode)event.xkey.keycode, 0, 0);
				
				Linux_ProcessKeyboardEvent(key, (event.type == KeyPress), event.xkey.state);
				
				//Platform_DebugLog("%i - %i - %i - %i - %i - %i\n", event.xkey.type, event.xkey.x, event.xkey.y, event.xkey.state, event.xkey.keycode, key);
			} break;
			case ButtonPress:
			case ButtonRelease:
			{
				Linux_ProcessMouseEvent(event.xbutton.button, (event.type == ButtonPress));
			} break;
		}
	}
	
	Linux_UpdateInputPos();
}

API void
Platform_FinishFrame(void)
{
	switch (global_graphics_context.api)
	{
		case GraphicsAPI_OpenGL: Linux_OpenGLSwapBuffers(); break;
		default: break;
	}
}

API void*
Platform_HeapAlloc(uintsize size)
{
	Trace("Platform_HeapAlloc");
	
	void* result = malloc(size);
	memset(result, 0, size);
	return result;
}

API void*
Platform_HeapRealloc(void* ptr, uintsize size)
{
	Trace("Platform_HeapRealloc");
	
	// TODO: zero memory
	return realloc(ptr, size);
}

API void
Platform_HeapFree(void* ptr)
{
	Trace("Platform_HeapFree");
	
	free(ptr);
}

API void*
Platform_VirtualReserve(uintsize size)
{
	Trace("Platform_VirtualReserve");
	
	void* memory = mmap(NULL, size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	
	return memory;
}

API void
Platform_VirtualCommit(void* ptr, uintsize size)
{
	Trace("Platform_VirtualCommit");
	
	mprotect(ptr, size, PROT_WRITE | PROT_READ);
}

API void
Platform_VirtualFree(void* ptr, uintsize size)
{
	Trace("Platform_VirtualFree");
	
	munmap(ptr, size);
}

API void*
Platform_ReadEntireFile(String path, uintsize* out_size, Arena* opt_arena)
{
	Trace("Platform_ReadEntireFile");
	
	char local_buffer[1 << 10];
	const char* buffer;
	
	if (path.len > 0 && path.data[path.len-1] == 0)
	{
		buffer = path.data;
	}
	else
	{
		snprintf(local_buffer, sizeof local_buffer, "%.*s", path.len, path.data);
	}
	
	FILE* file = fopen(buffer, "rb");
	if (!file)
		return NULL;
	
	fseek(file, 0, SEEK_END);
	uintsize size = (uintsize)ftell(file);
	rewind(file);
	
	void* memory = opt_arena ? Arena_Push(opt_arena, size) : Platform_HeapAlloc(size);
	if (!memory)
	{
		fclose(file);
		return NULL;
	}
	
	size = fread(memory, 1, size, file);
	fclose(file);
	
	*out_size = size;
	return memory;
}

API bool32
Platform_WriteEntireFile(String path, const void* data, uintsize size)
{
	Trace("Platform_WriteEntireFile");
	
	char local_buffer[1 << 10];
	const char* buffer;
	
	if (path.len > 0 && path.data[path.len-1] == 0)
	{
		buffer = path.data;
	}
	else
	{
		snprintf(local_buffer, sizeof local_buffer, "%.*s", path.len, path.data);
	}
	
	FILE* file = fopen(buffer, "wb");
	if (!file)
		return false;
	
	bool32 success = (size == fwrite(data, 1, size, file));
	fclose(file);
	
	return success;
}

API void
Platform_FreeFileMemory(void* ptr, uintsize size)
{
	// NOTE(ljre): Ignore parameter 'size' for this platform.
	
	Platform_HeapFree(ptr);
}

API uint64
Platform_CurrentPosixTime(void)
{
	time_t result = time(NULL);
	if (result == -1)
		result = 0;
	
	return (uint64)result;
}

//- Debug
#ifdef DEBUG
API void
Platform_DebugError(const char* msg)
{
	printf("======== Debug Error\n%s\n", msg);
	exit(1);
	/* no return */
}

API void
Platform_DebugMessageBox(const char* restrict format, ...)
{
	char buffer[16 << 10];
	
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	puts(buffer); // TODO
	va_end(args);
}

API void
Platform_DebugLog(const char* restrict format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	fflush(stdout);
}
#endif // DEBUG
