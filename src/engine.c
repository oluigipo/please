//
//
//
//

#define INTERNAL_COMPLETE_GRAPHICS_CONTEXT
#include "internal.h"

//~ Libraries
#define STB_IMAGE_STATIC
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_STDIO
#include "ext/stb_image.h"

#define STBTT_STATIC
#include "ext/stb_truetype.h"

#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_STATIC
#define STB_VORBIS_NO_STDIO
#include "ext/stb_vorbis.h"

//~ Our Code
internal Engine_Data global_engine;

// NOTE(ljre): The reference FPS 'Engine_Data.delta_time' is going to be based of.
//             60 FPS = 1.0 DT
#define REFERENCE_FPS 60
// NOTE(ljre): Optional. Only matters if you are going to use Discord Game SDK
#define DISCORD_APP_ID 0

#ifdef USE_DISCORD_GAME_SDK
#   include "engine_discord.c"
#endif

#include "engine_file_json.c"
#include "engine_file_gltf.c"
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
#   pragma GCC diagnostic ignored "-Wsign-conversion"
#else
#   pragma warning(push, 0)
#endif

//- Includes
#define STBI_MALLOC(sz) Platform_HeapAlloc(sz)
#define STBI_REALLOC(p,newsz) Platform_HeapRealloc(p,newsz)
#define STBI_FREE(p) Platform_HeapFree(p)
//#define STBI_MALLOC(sz) Engine_PushMemory(sz)
//#define STBI_REALLOC(p,newsz) Engine_RePushMemory(p,newsz)
//#define STBI_FREE(p) Engine_PopMemory(p)
#define STBI_ASSERT Assert

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STBTT_malloc(x,u) ((void)(u),Platform_HeapAlloc(x))
#define STBTT_free(x,u) ((void)(u),Platform_HeapFree(x))
//#define STBTT_malloc(x,u) ((void)(u),Engine_PushMemory(x))
//#define STBTT_free(x,u) ((void)(u),Engine_PopMemory(x))
#define STBTT_assert Assert

#define STB_TRUETYPE_IMPLEMENTATION
#include "ext/stb_truetype.h"

#undef STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_MAX_CHANNELS 2
#define malloc(sz) Platform_HeapAlloc(sz)
#define realloc(p,sz) Platform_HeapRealloc(p,sz)
#define free(p) Platform_HeapFree(p)
#include "ext/stb_vorbis.h"

#undef R
#undef L
#undef C

#undef malloc
#undef realloc
#undef free

//- Enable warnings
#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#else
#   pragma warning(pop, 0)
#endif
