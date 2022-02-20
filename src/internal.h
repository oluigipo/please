#ifndef INTERNAL_H
#define INTERNAL_H

#include "internal_config.h"

#define AlignUp(x, mask) (((x) + (mask)) & ~(mask))
#define IsPowerOf2(x) ( ((x) & (x)-1) == 0 )
#define Kilobytes(n) ((uintsize)(n) * 1024)
#define Megabytes(n) (1024*Kilobytes(n))
#define Gigabytes(n) (1024*Megabytes(n))
#define ArrayLength(x) (sizeof(x) / sizeof(*(x)))
#define PI64 3.141592653589793238462643383279
#define PI32 ((float32)PI64)
#define Min(x,y) ((x) < (y) ? (x) : (y))
#define Max(x,y) ((x) > (y) ? (x) : (y))

#if defined(__clang__)
#   define Assume(...) __builtin_assume(__VA_ARGS__)
#   define Debugbreak() __builtin_debugtrap()
#   define Likely(...) __builtin_expect(!!(__VA_ARGS__), 1)
#   define Unlikely(...) __builtin_expect((__VA_ARGS__), 0)
#   define Unreachable() __builtin_unreachable()
#elif defined(_MSC_VER)
#   define Assume(...) __assume(__VA_ARGS__)
#   define Debugbreak() __debugbreak()
#   define Likely(...) (__VA_ARGS__)
#   define Unlikely(...) (__VA_ARGS__)
#   define Unreachable() do { Assume(0); } while (0)
#elif defined(__GNUC__)
#   define Assume(...) do { if (!(__VA_ARGS__)) __builtin_unreachable(); } while (0)
#   define Debugbreak() __asm__ __volatile__ ("int $3")
#   define Likely(...) __builtin_expect(!!(__VA_ARGS__), 1)
#   define Unlikely(...) __builtin_expect((__VA_ARGS__), 0)
#   define Unreachable() __builtin_unreachable()
#else
#   define Assume(...) ((void)0)
#   define Debugbreak() ((void)0)
#   define Likely(...) (__VA_ARGS__)
#   define Unlikely(...) (__VA_ARGS__)
#   define Unreachable() ((void)0)
#endif

#if defined(DEBUG)
API void Platform_DebugError(const char* msg);
#   undef Unreachable
#   define Unreachable() do { Platform_DebugError("Unreachable code reached!\nFile: " __FILE__ "\nLine: " StrMacro(__LINE__)); } while (0)
#   define Assert(...) do { if (!(__VA_ARGS__)) { Debugbreak(); Platform_DebugError("Assertion Failure!\nFile: " __FILE__ "\nLine: " StrMacro(__LINE__) "\nExpression: " #__VA_ARGS__); } } while (0)
#else
#   define Assert(...) Assume(__VA_ARGS__)
#   undef Debugbreak
#   define Debugbreak() ((void)0)
#endif

//~ Standard headers & Libraries
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wall"
#   pragma GCC diagnostic ignored "-Wextra"
#else
#   pragma warning(push, 0)
#endif

//- Includes
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <cglm/cglm.h>
#include <stdbool.h>

//- Enable Warnings
#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#else
#   pragma warning(pop, 0)
#endif

//~ Types
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uintptr_t uintptr;
typedef size_t uintsize;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef intptr_t intptr;
typedef ptrdiff_t intsize;

typedef int8_t bool8;
typedef int16_t bool16;
typedef int32_t bool32;
typedef int64_t bool64;

typedef float float32;
typedef double float64;

//~ Debug
#if defined(TRACY_ENABLE) && defined(__clang__) // NOTE(ljre): this won't work with MSVC...
#    include "../../../ext/tracy/TracyC.h"
internal inline void ___my_tracy_zone_end(TracyCZoneCtx* ctx) { TracyCZoneEnd(*ctx); }
#    define TraceCat__(x,y) x ## y
#    define TraceCat_(x,y) TraceCat__(x,y)
#    define Trace(x) TracyCZoneN(TraceCat_(_ctx,__LINE__) __attribute((cleanup(___my_tracy_zone_end))),x,true)
#    define TraceFrameMark() TracyCFrameMark
#else
#    define Trace(x) ((void)0)
#    define TraceFrameMark() ((void)0)
#endif

#include "internal_memory.h"
#include "internal_string.h"
#include "internal_arena.h"
#include "internal_assets.h"

#include "internal_api_engine.h"
#include "internal_api_render.h"
#include "internal_api_platform.h"

#endif //INTERNAL_H
