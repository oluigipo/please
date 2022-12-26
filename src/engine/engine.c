/*
*
* TODO:
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
#   include "internal_graphics_opengl.h"
#endif
#ifdef INTERNAL_ENABLE_D3D11
#   include "internal_graphics_d3d11.h"
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
static Engine_Data global_engine;

#include "file_formats/json.h"
#include "file_formats/qoi.h"
#include "file_formats/gltf.h"

#include "engine_audio.c"
#ifdef INTERNAL_ENABLE_OPENGL
#   ifdef INTERNAL_TEST_OPENGL_NEWREN
#       include "engine_render_opengl.new.c"
#   else
#       include "engine_render_opengl.c"
#   endif
#endif
#ifdef INTERNAL_ENABLE_D3D11
#   include "engine_render_d3d11.c"
#endif
#include "engine_render.c"
#include "engine_main.c"

//~ External
DisableWarnings();

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

ReenableWarnings();
