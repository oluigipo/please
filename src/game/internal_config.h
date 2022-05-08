#ifndef INTERNAL_CONFIG_H
#define INTERNAL_CONFIG_H

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

#endif //INTERNAL_CONFIG_H
