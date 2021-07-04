#if defined(OS_WINDOWS)
#   include "platform_win32.c"
#elif defined(OS_LINUX)
#   include "platform_linux.c" // NOTE(ljre): Not done yet
#elif defined(OS_MAC)
#   include "platform_mac.c" // NOTE(ljre): Not done yet
#else
#   error You need to define a platform macro. It can be one of those: OS_WINDOWS, OS_LINUX, or OS_MAC
#endif

#include "game.c"
#include "engine.c"
