#ifndef INTERNAL_H
#define INTERNAL_H

#ifdef __GNUC__
#   define alignas(_) __attribute__((aligned(_)))
#else
#   define alignas(_) __declspec(align(_))
#endif

#if 0 // NOTE(ljre): Ignore this.
;
#define restrict
#define NULL
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#   define _CRT_SECURE_NO_WARNINGS
#endif

#define internal static
#define true 1
#define false 0

#if defined(UNITY_BUILD)
#   define API static
#elif defined(__cplusplus)
#   define API extern "C"
#else
#   define API
#endif

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
#ifndef DEBUG
#   define Assert(x) ((void)0)
#else
#   undef NDEBUG
#   include <assert.h>
#   define Assert assert
#endif

#if defined(TRACY_ENABLE) && defined(__clang__) // NOTE(ljre): this won't work with MSVC...
#    include "../../tracy/TracyC.h"
internal inline void ___my_tracy_zone_end(TracyCZoneCtx* ctx) { TracyCZoneEnd(*ctx); }
#    define Trace(x) TracyCZoneN(_ctx __attribute((cleanup(___my_tracy_zone_end))),x,true)
#else
#    define Trace(x) ((void)0)
#endif

#include "internal_arena.h"
#include "internal_string.h"
#include "internal_opengl.h"
#include "internal_direct3d.h"
#include "internal_assets.h"

#include "internal_api_engine.h"
#include "internal_api_render.h"
#include "internal_api_platform.h"

#endif //INTERNAL_H
