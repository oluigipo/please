//
//
//
//

#include "internal.h"

//- Libraries
#define STB_IMAGE_STATIC
#include "ext/stb_image.h"
#define STBTT_STATIC
#include "ext/stb_truetype.h"

//- Files
#include "engine_main.c"

//- External
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wsign-conversion"
#   pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#   pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-conversion"
#   pragma GCC diagnostic ignored "-Wconversion"
#else
#   pragma warning(push, 0)
#endif

#define STBI_MALLOC Platform_HeapAlloc
#define STBI_REALLOC Platform_HeapRealloc
#define STBI_FREE Platform_HeapFree

#define STB_IMAGE_IMPLEMENTATION
//#define STBI_NO_THREAD_LOCALS
#include "ext/stb_image.h"

#define STBTT_malloc(x,u) ((void)(u),Platform_HeapAlloc(x))
#define STBTT_free(x,u) ((void)(u),Platform_HeapFree(x))

#define STB_TRUETYPE_IMPLEMENTATION
#include "ext/stb_truetype.h"

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#else
#   pragma warning(pop, 0)
#endif
