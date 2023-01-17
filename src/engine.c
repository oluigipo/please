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

#include "api_engine.h"

#ifdef CONFIG_ENABLE_OPENGL
#   include "api_os_opengl.h"
#endif
#ifdef CONFIG_ENABLE_D3D11
#   include "api_os_d3d11.h"
#endif

//~ Libraries
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include <ext/stb_image.h>

#define STBTT_STATIC
#include <ext/stb_truetype.h>

#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_STATIC
#define STB_VORBIS_NO_STDIO
#include <ext/stb_vorbis.h>

//~ Our Code
static Engine_Data global_engine;

#include "fileformat_json.h"
#include "fileformat_qoi.h"
#include "fileformat_gltf.h"

#ifdef CONFIG_ENABLE_OPENGL
#   include "engine_render_opengl.c"
#endif

#ifdef CONFIG_ENABLE_D3D11
#   include "engine_render_d3d11.c"
#endif

#include "engine_audio.c"
#include "engine_render.c"
#include "engine_main.c"

#include "renderbackend.c"

//~ External
DisableWarnings();

#define STBI_MALLOC(sz) OS_HeapAlloc(sz)
#define STBI_REALLOC(p,newsz) OS_HeapRealloc(p,newsz)
#define STBI_FREE(p) OS_HeapFree(p)
#define STBI_ASSERT Assert

#define STB_IMAGE_IMPLEMENTATION
#include <ext/stb_image.h>

#define STBTT_malloc(x,u) ((void)(u),OS_HeapAlloc(x))
#define STBTT_free(x,u) ((void)(u),OS_HeapFree(x))
#define STBTT_assert Assert

#define STB_TRUETYPE_IMPLEMENTATION
#include <ext/stb_truetype.h>

#undef STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_MAX_CHANNELS 2
#define malloc(sz) OS_HeapAlloc(sz)
#define realloc(p,sz) OS_HeapRealloc(p,sz)
#define free(p) OS_HeapFree(p)
#include <ext/stb_vorbis.h>

#undef R
#undef L
#undef C

#undef malloc
#undef realloc
#undef free

ReenableWarnings();
