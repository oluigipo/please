#ifndef API_DEBUGTOOLS_H
#define API_DEBUGTOOLS_H

#include "api_engine.h"

struct DBG_UIState
{
	E_GlobalData* engine;
	E_RectBatch* batch;
	E_Font* font;
	E_RectBatchElem* deferred_background;
	vec2 scale;
	
	vec2 start_pos;
	vec2 current_pos;
	
	float32 xoffset;
	float32 max_width;
	float32 tab_size;
	float32 elems_spacing;
}
typedef DBG_UIState;

API DBG_UIState DBG_UIBegin(E_GlobalData* engine, E_RectBatch* batch, E_Font* font, vec2 pos, vec2 scale);
API bool DBG_UIPushFoldable(DBG_UIState* state, String text, bool* is_unfolded);
API void DBG_UIPopFoldable(DBG_UIState* state);
API void DBG_UIPushTextF(DBG_UIState* state, const char* fmt, ...);
API bool DBG_UIPushButton(DBG_UIState* state, String text);
API void DBG_UIPushSlider(DBG_UIState* state, float32 min, float32 max, float32* value);
API void DBG_UIPushProgressBar(DBG_UIState* state, float32 width, float32* ts, vec3* colors, int32 bar_count);
API void DBG_UIPushArenaInfo(DBG_UIState* state, Arena* arena, String optional_name);
API void DBG_UIPushVerticalSpacing(DBG_UIState* state, float32 spacing);
API String DBG_UIPushTextField(DBG_UIState* state, uint8* buffer, intsize buffer_cap, intsize* buffer_size, bool* is_selected, float32 min_width);
API void DBG_UIEnd(DBG_UIState* state);

#endif //API_DEBUGTOOLS_H
