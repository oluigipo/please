//
//
//
//

#include "internal.h"

//~ Libraries
#define STB_IMAGE_STATIC
#include "ext/stb_image.h"
#define STBTT_STATIC
#include "ext/stb_truetype.h"
#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_STATIC
#define STB_VORBIS_NO_STDIO
#include "ext/stb_vorbis.h"

//~ Files
#include "engine_random.c"
#include "engine_audio.c"
#include "engine_render.c"
#include "engine_main.c"

//~ External
//  Disable Warnings
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wall"
#   pragma GCC diagnostic ignored "-Wextra"
#else
#   pragma warning(push, 0)
#endif

//- Includes
#define STBI_MALLOC Platform_HeapAlloc
#define STBI_REALLOC Platform_HeapRealloc
#define STBI_FREE Platform_HeapFree

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STBTT_malloc(x,u) ((void)(u),Platform_HeapAlloc(x))
#define STBTT_free(x,u) ((void)(u),Platform_HeapFree(x))

#define STB_TRUETYPE_IMPLEMENTATION
#include "ext/stb_truetype.h"

#undef STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_MAX_CHANNELS 2
#include "ext/stb_vorbis.h"

//- Enable warnings
#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#else
#   pragma warning(pop, 0)
#endif
