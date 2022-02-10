/*
*
* TODO:
*  - Resize Window;
*  - Fullscreen;
*  - Keyboard for 3D Demo;
*  - Direct3D backend;
*  - Systems:
*      - Basic rect collision;
*
*/

//
//
//
//

#include "internal.h"
#ifdef INTERNAL_ENABLE_OPENGL
#   include "internal_opengl.h"
#endif
#ifdef INTERNAL_ENABLE_D3D11
#   include "internal_direct3d.h"
#endif

//~ Libraries
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
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

#include "engine_file_qoi.c"
#include "engine_file_json.c"
#include "engine_file_gltf.c"
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
#define STBI_ASSERT Assert

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STBTT_malloc(x,u) ((void)(u),Platform_HeapAlloc(x))
#define STBTT_free(x,u) ((void)(u),Platform_HeapFree(x))
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
