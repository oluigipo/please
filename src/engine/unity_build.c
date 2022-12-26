#ifndef _CRT_SECURE_NO_WARNINGS
#   define _CRT_SECURE_NO_WARNINGS
#endif

#include "internal.h"

#include "engine.c"

#if defined(_WIN32)
#   include "platform_win32.c"
#elif defined(__linux__)
#   include "platform_linux.c" // NOTE(ljre): Not done yet
#elif defined(__APPLE__)
#   include "platform_mac.c" // NOTE(ljre): Not done yet
#else
#   error Could not identify the platform you are building to. Check the 'src/unity_build.c' file.
#endif
