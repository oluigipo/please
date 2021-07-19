#ifndef UNITY_BUILD
#   define UNITY_BUILD
#endif

#if defined(_WIN32)
#   include "platform_win32.c"
#elif defined(__linux__)
#   include "platform_linux.c" // NOTE(ljre): Not done yet
#elif defined(__APPLE__)
#   include "platform_mac.c" // NOTE(ljre): Not done yet
#else
#   error You need to define a platform macro. It can be one of those: OS_WINDOWS, OS_LINUX, or OS_MAC
#endif

#include "game.c"
#include "engine.c"
