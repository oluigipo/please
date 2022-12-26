#ifndef INTERNAL_H
#define INTERNAL_H

#define INTERNAL_TEST_OPENGL_NEWREN

#include "internal_config.h"

//~ Common
#define COMMON_DONT_USE_CRT

#include "common_defs.h"
#include "common_types.h"

API void* Platform_VirtualReserve(uintsize size);
API void Platform_VirtualCommit(void* ptr, uintsize size);
API void Platform_VirtualFree(void* ptr, uintsize size);
#define Arena_OsReserve_(size) Platform_VirtualReserve(size)
#define Arena_OsCommit_(ptr, size) Platform_VirtualCommit(ptr, size)
#define Arena_OsFree_(ptr, size) Platform_VirtualFree(ptr, size)

#include "common.h"

//~ Debug
#if defined(DEBUG)
API void Platform_DebugError(const char* fmt, ...);

#   ifdef _WIN32
extern int32 __stdcall IsDebuggerPresent(void);
#       undef Debugbreak
#       define Debugbreak() do { if (IsDebuggerPresent()) __debugbreak(); } while (0)
#   endif

#   undef Unreachable
#   undef Assert
#   define Unreachable() do { Debugbreak(); Platform_DebugError("Unreachable code reached!\nFile: " __FILE__ "\nLine: " StrMacro(__LINE__) "\nFunction: %s", __func__); } while (0)
#   define Assert(...) do { if (!(__VA_ARGS__)) { Debugbreak(); Platform_DebugError("Assertion Failure!\nFile: " __FILE__ "\nLine: " StrMacro(__LINE__) "\nFunction: %s\nExpression: " #__VA_ARGS__, __func__); } } while (0)
#else
#   undef Assert
#   undef Debugbreak
#   define Assert(...) Assume(__VA_ARGS__)
#   define Debugbreak() ((void)0)
#endif

//~ Libraries
DisableWarnings();
#include "ext/cglm/cglm.h"
ReenableWarnings();

//~ Tracy
#if defined(TRACY_ENABLE) && defined(__clang__) // NOTE(ljre): this won't work with MSVC...
#    include "../../../ext/tracy/TracyC.h"
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

//~ More Headers
#include "internal_assets.h"

#include "internal_api_engine.h"
#include "internal_api_render.h"
#include "internal_api_audio.h"
#include "internal_api_platform.h"

#endif //INTERNAL_H
