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
#define STB_VORBIS_NO_INTEGER_CONVERSION
#include <ext/stb_vorbis.h>

//~ Our Code
#include "api_engine.h"
#include "api_steam.h"

static E_GlobalData global_engine;

#include "util_json.h"
#include "util_qoi.h"
#include "util_gltf.h"

#include "engine_audio.c"
#include "engine_thread.c"
#include "engine_render.c"
#include "engine_main.c"

#include "renderbackend.c"

//~ External
DisableWarnings();

#define memcpy Mem_Copy
#define memset Mem_Set

#define STBI_MALLOC(sz) OS_HeapAlloc(sz)
#define STBI_REALLOC(p,newsz) OS_HeapRealloc(p,newsz)
#define STBI_FREE(p) OS_HeapFree(p)
#define STBI_ASSERT Assert
#define STB_IMAGE_IMPLEMENTATION
#include <ext/stb_image.h>

#define STBTT_malloc(x,u) ((u) ? Arena_PushDirtyAligned(u, x, 16) : OS_HeapAlloc(x))
#define STBTT_free(x,u) ((u) ? (void)(x) : OS_HeapFree(x))
#define STBTT_assert Assert
#define STBTT_strlen Mem_Strlen
#define STBTT_memcpy Mem_Copy
#define STBTT_memset Mem_Set
#define STB_TRUETYPE_IMPLEMENTATION
#include <ext/stb_truetype.h>

#undef STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_MAX_CHANNELS 8
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
