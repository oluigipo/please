#ifndef API_H
#define API_H

#include "config.h"
#include "engine_internal_config.h"

#include "api_os.h"
#include "api_renderbackend.h"

//~ Debug
#if defined(CONFIG_DEBUG)
#   ifdef _WIN32
extern __declspec(dllimport) int __stdcall IsDebuggerPresent(void);
#       undef Debugbreak
#       define Debugbreak() do { if (IsDebuggerPresent()) __debugbreak(); } while (0)
#   endif
#   undef Unreachable
#   undef Assert
#   define Unreachable() do { Debugbreak(); OS_ExitWithErrorMessage("Unreachable code reached!\nFile: " __FILE__ "\nLine: " StrMacro(__LINE__) "\nFunction: %s", __func__); } while (0)
#   define Assert(...) do { if (!(__VA_ARGS__)) { Debugbreak(); OS_ExitWithErrorMessage("Assertion Failure!\nFile: " __FILE__ "\nLine: " StrMacro(__LINE__) "\nFunction: %s\nExpression: " #__VA_ARGS__, __func__); } } while (0)
#else
#   undef Assert
#   undef Debugbreak
#   define Assert(...) Assume(__VA_ARGS__)
#   define Debugbreak() ((void)0)
#endif

//~ Libraries
DisableWarnings();
#include <ext/cglm/cglm.h>
#define STBTT_STATIC
#include <ext/stb_truetype.h>
ReenableWarnings();

//~ More Headers
struct Asset_SoundBuffer
{
	int32 channels;
	int32 sample_rate;
	int32 sample_count;
	int16* samples;
}
typedef Asset_SoundBuffer;

#include "engine_internal_api_engine.h"
#include "engine_internal_api_render.h"

#endif //API_H
