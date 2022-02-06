#ifndef INTERNAL_CONFIG_H
#define INTERNAL_CONFIG_H

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

#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#   define _CRT_SECURE_NO_WARNINGS
#endif

#define internal static
#define true 1
#define false 0

#if defined(UNITY_BUILD)
#   define API static
#elif defined(__cplusplus)
#   define API extern "C"
#elif defined(INTERNAL_ENABLE_HOT)
#   ifndef _WIN32
#       error Hot reloading only available on Windows.
#   endif
#   define API __declspec(dllexport)
#else
#   define API
#endif

#ifndef INTERNAL_ENABLE_OPENGL
#   define INTERNAL_ENABLE_OPENGL
#endif
#if !defined(INTERNAL_ENABLE_D3D11) && defined(_WIN32)
#   define INTERNAL_ENABLE_D3D11
#endif

#if 0 // NOTE(ljre): Never enabled by default.
#   define INTERNAL_ENABLE_DISCORD_SDK
#endif

#endif //INTERNAL_CONFIG_H
