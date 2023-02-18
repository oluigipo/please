#include "config.h"
#include "api_os.h"
#include "api_engine.h"

#if defined(CONFIG_OSLAYER_WIN32)
#   include "os_win32.c"
#elif defined(CONFIG_OSLAYER_LINUX)
#   include "os_linux.c"
#elif defined(CONFIG_OSLAYER_SDL)
#   include "os_sdl.c"
#endif
