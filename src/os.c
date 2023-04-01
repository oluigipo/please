#include "config.h"
#include "api_os.h"
#include "api_engine.h"

#if defined(CONFIG_OS_WIN32) || defined(CONFIG_OS_WIN32LINUX)
#   include "os_win32.c"
#elif defined(CONFIG_OS_LINUX)
#   include "os_linux.c"
#elif defined(CONFIG_OS_SDL2LINUX)
#   include "os_sdl2.c"
#elif defined(CONFIG_OS_ANDROID)
#   include "os_android.c"
#endif
