#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#   define _CRT_SECURE_NO_WARNINGS
#endif

#define internal static
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
#   define DisableWarnings() \
_Pragma("clang diagnostic push")\
_Pragma("clang diagnostic ignored \"-Weverything\"")
#   define ReenableWarnings() \
_Pragma("clang diagnostic pop")
#elif defined(_MSC_VER)
#   define Assume(...) __assume(__VA_ARGS__)
#   define Debugbreak() __debugbreak()
#   define Likely(...) (__VA_ARGS__)
#   define Unlikely(...) (__VA_ARGS__)
#   define Unreachable() do { Assume(0); } while (0)
#   define DisableWarnings() \
__pragma(warning(push, 0))
#   define ReenableWarnings() \
__pragma(warning(pop))
#elif defined(__GNUC__)
#   define Assume(...) do { if (!(__VA_ARGS__)) __builtin_unreachable(); } while (0)
#   define Debugbreak() __asm__ __volatile__ ("int $3")
#   define Likely(...) __builtin_expect(!!(__VA_ARGS__), 1)
#   define Unlikely(...) __builtin_expect((__VA_ARGS__), 0)
#   define Unreachable() __builtin_unreachable()
#   define DisableWarnings() \
_Pragma("GCC diagnostic push")\
_Pragma("GCC diagnostic ignored \"-Wall\"")\
_Pragma("GCC diagnostic ignored \"-Wextra\"")
#   define ReenableWarnings() \
_Pragma("GCC diagnostic pop")
#else
#   define Assume(...) ((void)0)
#   define Debugbreak() ((void)0)
#   define Likely(...) (__VA_ARGS__)
#   define Unlikely(...) (__VA_ARGS__)
#   define Unreachable() ((void)0)
#   define DisableWarnings()
#   define ReenableWarnings()
#endif

#define Assert(...) do { if (!(__VA_ARGS__)) { Debugbreak(); Unreachable(); } } while (0)

#if 0 // NOTE(ljre): Ignore this.
#define NULL
#endif

#ifdef __cplusplus
#   define restrict
#else
#   define alignas(x) _Alignas(x)
#   define alignof(x) _Alignof(x)
#endif

#endif //COMMON_DEFS_H
