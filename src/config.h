#ifndef CONFIG_H
#define CONFIG_H

//#define CONFIG_DEBUG
#define COMMON_DONT_USE_CRT
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

#if defined(_WIN32)
#   define _CRT_SECURE_NO_WARNINGS
#   define CONFIG_ENABLE_OPENGL
#   define CONFIG_ENABLE_D3D11
#elif defined(__linux__)
#   define CONFIG_ENABLE_OPENGL
#endif

//- Tracy
#if defined(TRACY_ENABLE) && defined(__clang__) // NOTE(ljre): this won't work with MSVC...
#    include "B:/ext/tracy/public/tracy/TracyC.h"
static inline void ___my_tracy_zone_end(TracyCZoneCtx* ctx) { TracyCZoneEnd(*ctx); }
#    define TraceCat__(x,y) x ## y
#    define TraceCat_(x,y) TraceCat__(x,y)
#    define Trace() TracyCZone(TraceCat_(_ctx,__LINE__) __attribute((cleanup(___my_tracy_zone_end))),true); ((void)TraceCat_(_ctx,__LINE__))
#    define TraceName(...) do { String a = (__VA_ARGS__); TracyCZoneName(TraceCat_(_ctx,__LINE__), (const char*)a.data, a.size); } while (0)
#    define TraceText(...) do { String a = (__VA_ARGS__); TracyCZoneText(TraceCat_(_ctx,__LINE__), (const char*)a.data, a.size); } while (0)
#    define TraceColor(...) TracyCZoneColor(TraceCat_(_ctx,__LINE__), (__VA_ARGS__))
#    define TraceF(sz, ...) do { char buf[sz]; uintsize len = String_PrintfBuffer(buf, sizeof(buf), __VA_ARGS__); TracyCZoneText(TraceCat_(_ctx,__LINE__), buf, len); } while (0)
#    define TraceFrameMark() TracyCFrameMark
#else
#    define Trace() ((void)0)
#    define TraceName(...) ((void)0)
#    define TraceText(...) ((void)0)
#    define TraceColor(...) ((void)0)
#    define TraceF(sz, ...) ((void)0)
#    define TraceFrameMark() ((void)0)
#endif

#endif //CONFIG_H
