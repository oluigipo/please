#include "internal.h"

#undef internal

#define __USE_MISC
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include <GL/glx.h>
#include <dlfcn.h>
#include <dirent.h>
#include <fcntl.h>

#define internal static

//~ Globals
internal Display* global_display;
internal Window global_window;

internal bool32 global_window_should_close;
internal int32 global_window_width;
internal int32 global_window_height;
internal GraphicsContext global_graphics_context;

//~ Functions
#include "platform_linux_input.c"
#include "platform_linux_audio.c"
#include "platform_linux_opengl.c"

//~ Entry Point
int32 main(int32 argc, char* argv[])
{
    Trace("main - Program Entry Point");
    
    int32 result = Engine_Main(argc, argv);
    
    if (global_display)
        XCloseDisplay(global_display);
    
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
Platform_CreateWindow(int32 width, int32 height, String name, uint32 flags, const GraphicsContext** out_graphics)
{
    Trace("Platform_CreateWindow");
    char local_buffer[Kilobytes(1)];
    const char* title;
    
    if (name.len > 0 && name.data[name.len-1] == 0)
    {
        title = name.data;
    }
    else
    {
        snprintf(local_buffer, sizeof local_buffer, "%.*s", name.len, name.data);
        title = local_buffer;
    }
    
    global_display = XOpenDisplay(NULL);
    if (!global_display)
        return false;
    
    bool32 ok = false;
    
    ok = ok || (flags & GraphicsAPI_OpenGL && Linux_CreateOpenGLWindow(width, height, title));
    
    if (ok)
    {
        global_window_width = width;
        global_window_height = height;
        
        *out_graphics = &global_graphics_context;
        
        Linux_InitInput();
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

API float64
Platform_GetTime(void)
{
    // TODO
    
    return 0.0;
}

API void
Platform_PollEvents(void)
{
    Linux_UpdateInputPre();
    
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
Platform_VirtualAlloc(uintsize size)
{
    Trace("Platform_VirtualAlloc");
    
    void* memory = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    
    return memory;
}

API void
Platform_VirtualCommit(void* ptr, uintsize size)
{
    Trace("Platform_VirtualCommit");
    
    // NOTE(ljre): No need to commit!
}

API void
Platform_VirtualFree(void* ptr, uintsize size)
{
    Trace("Platform_VirtualFree");
    
    munmap(ptr, size);
}

API void*
Platform_ReadEntireFile(String path, uintsize* out_size)
{
    char local_buffer[Kilobytes(1)];
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
    
    void* memory = Platform_HeapAlloc(size);
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
    char local_buffer[Kilobytes(1)];
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
Platform_DebugMessageBox(const char* restrict format, ...)
{
    char buffer[Kilobytes(16)];
    
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof buffer, format, args);
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

API void
Platform_DebugDumpBitmap(const char* restrict path, const void* data, int32 width, int32 height, int32 channels)
{
    FILE* file = fopen(path, "w");
    if (!file)
        return;
    
    fprintf(file, "P3\n%i %i 255\n", width, height);
    int32 size = width*height;
    const uint8* pixels = data;
    
    for (int32 i = 0; i < size; ++i)
    {
        int32 r, g, b;
        
        switch (channels)
        {
            case 1:
            {
                r = *pixels;
                g = *pixels;
                b = *pixels;
                
                ++pixels;
            } break;
            
            case 2:
            {
                r = pixels[0];
                g = pixels[1];
                b = 0;
                
                pixels += 2;
            } break;
            
            case 3:
            {
                r = pixels[0];
                g = pixels[1];
                b = pixels[2];
                
                pixels += 3;
            } break;
        }
        
        fprintf(file, "%i %i %i ", r, g, b);
    }
    
    fclose(file);
}
#endif // DEBUG
