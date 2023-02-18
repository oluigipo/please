#ifndef CONFIG_H
#define CONFIG_H

//#define CONFIG_DEBUG
#ifdef CONFIG_DEBUG
#   define COMMON_DEBUG
#endif

#if defined(__cplusplus)
#   define API extern "C"
#elif defined(CONFIG_ENABLE_HOT)
#   ifndef _WIN32
#       error Hot reloading only available on Windows.
#   endif
#   define API __declspec(dllexport)
#else
#   define API
#endif

//- Identify platform if build system didn't tell us
#if !defined(CONFIG_OSLAYER_WIN32) && !defined(CONFIG_OSLAYER_LINUX) && !defined(CONFIG_OSLAYER_SDL)
#   if defined(_WIN32)
#       define CONFIG_OSLAYER_WIN32
#   elif defined(__linux__)
#       define CONFIG_OSLAYER_LINUX
#   else
#       error "unsupported system"
#   endif
#endif

//- Platform specific stuff
#if defined(CONFIG_OSLAYER_WIN32)
#   define _CRT_SECURE_NO_WARNINGS
#   define CONFIG_ENABLE_OPENGL
#   define CONFIG_ENABLE_D3D11
#   define CONFIG_ENABLE_D3D9C
#   ifdef _WIN64
#       define COMMON_DONT_USE_CRT
#   endif
#elif defined(CONFIG_OSLAYER_LINUX)
#   define CONFIG_ENABLE_OPENGL
#endif

//- Tracy
#if defined(TRACY_ENABLE) && defined(__clang__) // NOTE(ljre): this won't work with MSVC...
#    define TRACY_MANUAL_LIFETIME
#    define TRACY_DELAYED_INIT
#    include "B:/ext/tracy/public/tracy/TracyC.h"
static inline void ___my_tracy_zone_end(TracyCZoneCtx* ctx) { TracyCZoneEnd(*ctx); }
#    define TraceCat__(x,y) x ## y
#    define TraceCat_(x,y) TraceCat__(x,y)
#    define Trace() TracyCZone(TraceCat_(_ctx,__LINE__) __attribute((cleanup(___my_tracy_zone_end))),true); ((void)TraceCat_(_ctx,__LINE__))
#    define TraceName(...) do { String a = (__VA_ARGS__); TracyCZoneName(TraceCat_(_ctx,__LINE__), (const char*)a.data, a.size); } while (0)
#    define TraceText(...) do { String a = (__VA_ARGS__); TracyCZoneText(TraceCat_(_ctx,__LINE__), (const char*)a.data, a.size); } while (0)
#    define TraceColor(...) TracyCZoneColor(TraceCat_(_ctx,__LINE__), (__VA_ARGS__))
#    define TraceF(sz, ...) do { char buf[sz]; uintsize len = String_PrintfBuffer(buf, sizeof(buf), __VA_ARGS__); TracyCZoneText(TraceCat_(_ctx,__LINE__), buf, len); } while (0)
#    define TraceFrameBegin() TracyCFrameMarkStart(0)
#    define TraceFrameEnd() TracyCFrameMarkEnd(0)
#    define TraceInit() ___tracy_startup_profiler()
#    define TraceDeinit() ___tracy_shutdown_profiler()
#else
#    define Trace() ((void)0)
#    define TraceName(...) ((void)0)
#    define TraceText(...) ((void)0)
#    define TraceColor(...) ((void)0)
#    define TraceF(sz, ...) ((void)0)
#    define TraceFrameBegin() ((void)0)
#    define TraceFrameEnd() ((void)0)
#    define TraceInit() ((void)0)
#    define TraceDeinit() ((void)0)
#endif

//- IncludeBinary

// https://gist.github.com/mmozeiko/ed9655cf50341553d282
#if defined(CONFIG_ENABLE_EMBED) && (defined(__clang__) || defined(__GNUC__))

#ifdef _WIN32
#   define IncludeBinary_Section ".rdata, \"dr\""
#   ifdef _WIN64
#       define IncludeBinary_Name(name) #name
#   else
#       define IncludeBinary_Name(name) "_" #name
#   endif
#else
#   define IncludeBinary_Section ".rodata"
#   define IncludeBinary_Name(name) #name
#endif

// this aligns start address to 16 and terminates byte array with explict 0
// which is not really needed, feel free to change it to whatever you want/need
#define IncludeBinary(name, file) \
__asm__(".section " IncludeBinary_Section "\n" \
".global " IncludeBinary_Name(name) "_begin\n" \
".balign 16\n" \
IncludeBinary_Name(name) "_begin:\n" \
".incbin \"" file "\"\n" \
\
".global " IncludeBinary_Name(name) "_end\n" \
".balign 1\n" \
IncludeBinary_Name(name) "_end:\n" \
".byte 0\n" \
); \
extern __attribute__((aligned(16))) const unsigned char name ## _begin[]; \
extern                              const unsigned char name ## _end[]

#endif //defined(CONFIG_ENABLE_EMBED) && (defined(__clang__) || defined(__GNUC__))

#endif //CONFIG_H
