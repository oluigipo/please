#include "config.h"
#include "api_os.h"

#ifdef _WIN32
#   include "os_win32.c"
#elif defined(__linux__)
#   include "os_linux.c"
#else
#   error could not identify OS.
#endif
