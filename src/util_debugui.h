#ifndef UTIL_DEBUGUI_H
#define UTIL_DEBUGUI_H

struct UDebugUI_State
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
typedef UDebugUI_State;

static UDebugUI_State UDebugUI_Begin(E_GlobalData* engine, E_RectBatch* batch, E_Font* font, vec2 pos, vec2 scale);
static bool UDebugUI_PushFoldable(UDebugUI_State* state, String text, bool* is_unfolded);
static void UDebugUI_PopFoldable(UDebugUI_State* state);
static void UDebugUI_PushTextF(UDebugUI_State* state, const char* fmt, ...);
static bool UDebugUI_PushButton(UDebugUI_State* state, String text);
static void UDebugUI_PushSlider(UDebugUI_State* state, float32 min, float32 max, float32* value);
static void UDebugUI_PushProgressBar(UDebugUI_State* state, float32 width, float32* ts, vec3* colors, int32 bar_count);
static void UDebugUI_PushArenaInfo(UDebugUI_State* state, Arena* arena, String optional_name);
static void UDebugUI_PushVerticalSpacing(UDebugUI_State* state, float32 spacing);
static void UDebugUI_End(UDebugUI_State* state);

//~ Implementation

static UDebugUI_State
UDebugUI_Begin(E_GlobalData* engine, E_RectBatch* batch, E_Font* font, vec2 pos, vec2 scale)
{
	UDebugUI_State state = {
		.engine = engine,
		.batch = batch,
		.font = font,
		.deferred_background = Arena_PushStruct(batch->arena, E_RectBatchElem),
		.scale = { scale[0], scale[1] },
		.start_pos = { pos[0], pos[1] },
		.current_pos = { pos[0] + 4.0f, pos[1] + 4.0f },
		.xoffset = 16.0f,
		.tab_size = 16.0f,
		.elems_spacing = 2.0f,
	};
	
	++batch->count;
	
	return state;
}

static bool
UDebugUI_PushFoldable(UDebugUI_State* state, String text, bool* is_unfolded)
{
	const float32 dot_size = 32.0f * glm_min(state->scale[0], state->scale[1]);
	const float32 dot_pad = 4.0f * glm_min(state->scale[0], state->scale[1]);
	const float32 dot_color = *is_unfolded ? 0.8f : 0.1f;
	
	vec2 size = { 0 };
	E_CalcTextSize(state->font, text, state->scale, &size);
	state->max_width = glm_max(size[0] + state->xoffset + dot_size, state->max_width);
	
	vec2 mouse;
	glm_vec2_copy(state->engine->os->input.mouse.pos, mouse);
	if (OS_IsPressed(state->engine->os->input.mouse, OS_MouseButton_Left) &&
		mouse[0] >= state->current_pos[0] && state->current_pos[0] + size[0] + dot_size >= mouse[0] &&
		mouse[1] >= state->current_pos[1] && state->current_pos[1] + size[1] >= mouse[1])
	{
		*is_unfolded ^= 1;
	}
	
	E_PushRect(state->batch, &(E_RectBatchElem) {
		.pos = {
			state->current_pos[0] + dot_pad,
			state->current_pos[1] + (size[1] - dot_size) + dot_pad,
		},
		.scaling[0][0] = dot_size - dot_pad*2,
		.scaling[1][1] = dot_size - dot_pad*2,
		.tex_kind = 3.0f,
		.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
		.color = { dot_color, dot_color, dot_color, 1.0f },
	});
	
	E_PushText(state->batch, state->font, text, vec2(state->current_pos[0] + dot_size, state->current_pos[1]), state->scale, GLM_VEC4_ONE);
	
	if (*is_unfolded)
	{
		state->xoffset += state->tab_size;
		state->current_pos[0] += state->tab_size;
	}
	
	state->current_pos[1] += size[1] + state->elems_spacing;
	
	return *is_unfolded;
}

static void
UDebugUI_PopFoldable(UDebugUI_State* state)
{
	state->xoffset -= state->tab_size;
	state->current_pos[0] -= state->tab_size;
}

static void
UDebugUI_PushTextF(UDebugUI_State* state, const char* fmt, ...)
{
	for Arena_TempScope(state->engine->scratch_arena)
	{
		va_list args;
		va_start(args, fmt);
		String text = Arena_VPrintf(state->engine->scratch_arena, fmt, args);
		va_end(args);
		
		vec2 size = { 0 };
		E_CalcTextSize(state->font, text, state->scale, &size);
		state->max_width = glm_max(size[0] + state->xoffset, state->max_width);
		
		E_PushText(state->batch, state->font, text, state->current_pos, state->scale, GLM_VEC4_ONE);
		state->current_pos[1] += size[1] + state->elems_spacing;
	}
}

static bool
UDebugUI_PushButton(UDebugUI_State* state, String text)
{
	vec2 size = { 0 };
	E_CalcTextSize(state->font, text, state->scale, &size);
	
	const float32 padding = 4.0f * glm_min(state->scale[0], state->scale[1]);
	float32 width = size[0] + padding*2;
	float32 height = size[1] + padding;
	
	vec4 bbox = {
		[0] = state->current_pos[0],
		[1] = state->current_pos[1],
		[2] = state->current_pos[0] + width,
		[3] = state->current_pos[1] + height,
	};
	
	state->max_width = glm_max(width + state->xoffset, state->max_width);
	
	vec2 mouse;
	glm_vec2_copy(state->engine->os->input.mouse.pos, mouse);
	bool is_over = (mouse[0] >= bbox[0] && bbox[2] >= mouse[0] && mouse[1] >= bbox[1] && bbox[3] >= mouse[1]);
	bool result = is_over && OS_IsReleased(state->engine->os->input.mouse, OS_MouseButton_Left);
	
	float32 color = is_over && OS_IsDown(state->engine->os->input.mouse, OS_MouseButton_Left) ? 0.3f : 0.2f;
	
	E_PushRect(state->batch, &(E_RectBatchElem) {
		.pos = { bbox[0], bbox[1] },
		.tex_kind = 3,
		.scaling[0][0] = width + padding,
		.scaling[1][1] = height,
		.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
		.color = { color, color, color, 1.0f },
	});
	
	E_PushText(state->batch, state->font, text, vec2(state->current_pos[0] + padding, state->current_pos[1] + padding*0.5f), state->scale, GLM_VEC4_ONE);
	
	state->current_pos[1] += height + state->elems_spacing;
	
	return result;
}

static void
UDebugUI_PushSlider(UDebugUI_State* state, float32 min, float32 max, float32* value)
{
	// TODO
}

static void
UDebugUI_PushProgressBar(UDebugUI_State* state, float32 width, float32* ts, vec3* colors, int32 bar_count)
{
	width *= state->scale[0];
	const float32 height = 8.0f * state->scale[1];
	
	state->max_width = glm_max(width + state->xoffset, state->max_width);
	
	E_PushRect(state->batch, &(E_RectBatchElem) {
		.pos = { state->current_pos[0], state->current_pos[1] },
		.tex_kind = 3,
		.scaling[0][0] = width,
		.scaling[1][1] = height,
		.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
		.color = { 0.2f, 0.2f, 0.2f, 1.0f },
	});
	
	for (int32 i = 0; i < bar_count; ++i)
	{
		E_PushRect(state->batch, &(E_RectBatchElem) {
			.pos = { state->current_pos[0], state->current_pos[1] },
			.tex_kind = 3,
			.scaling[0][0] = width * ts[i],
			.scaling[1][1] = height,
			.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
			.color = { colors[i][0], colors[i][1], colors[i][2], 1.0f },
		});
	}
	
	state->current_pos[1] += height + state->elems_spacing;
}

static char
UDebugUI_ChooseMemoryMetric_(uintsize count, float32* out_flt)
{
	char ch = 'B';
	intsize base = 1;
	
	if (count >= 1 << 30)
		ch = 'G', base = 1 << 30;
	else if (count >= 1 << 20)
		ch = 'M', base = 1 << 20;
	else if (count >= 1 << 10)
		ch = 'K', base = 1 << 10;
	
	*out_flt = (float32)count / (float32)base;
	return ch;
}

static void
UDebugUI_PushArenaInfo(UDebugUI_State* state, Arena* arena, String name)
{
	float32 f_offset, f_peak, f_commited, f_reserved;
	
	char me_offset = UDebugUI_ChooseMemoryMetric_(arena->offset, &f_offset);
	char me_peak = UDebugUI_ChooseMemoryMetric_(arena->peak, &f_peak);
	char me_commited = UDebugUI_ChooseMemoryMetric_(arena->commited, &f_commited);
	char me_reserved = UDebugUI_ChooseMemoryMetric_(arena->reserved, &f_reserved);
	
	UDebugUI_PushTextF(state, "Arena '%S':", name);
	state->xoffset += state->tab_size;
	state->current_pos[0] += state->tab_size;
	
	if (arena->page_size != 0 && arena->page_size != arena->reserved)
	{
		UDebugUI_PushTextF(state, "Offset: %.2f%c current, %.2f%c peak", f_offset, me_offset, f_peak, me_peak);
		UDebugUI_PushProgressBar(state, 300.0f,
		(float32[]) {
				(float32)arena->peak / (float32)arena->commited,
				(float32)arena->offset / (float32)arena->commited,
			},
		(vec3[]) {
				{ 0.8f, 0.8f, 0.1f },
				{ 0.1f, 0.8f, 0.1f },
			}, 2);
		
		UDebugUI_PushTextF(state, "Virtual: %.2f%c commited, %.2f%c reserved", f_commited, me_commited, f_reserved, me_reserved);
		UDebugUI_PushProgressBar(state, 300.0f,
		(float32[]) {
				(float32)arena->commited / (float32)arena->reserved,
			},
		(vec3[]) {
				{ 0.1f, 0.8f, 0.1f },
			}, 1);
	}
	else
	{
		UDebugUI_PushTextF(state, "Offset: %.2f%c current, %.2f%c peak\nMax: %.2f%c", f_offset, me_offset, f_peak, me_peak, f_commited, me_commited);
		UDebugUI_PushProgressBar(state, 300.0f,
		(float32[]) {
				(float32)arena->peak / (float32)arena->commited,
				(float32)arena->offset / (float32)arena->commited,
			},
		(vec3[]) {
				{ 0.8f, 0.8f, 0.1f },
				{ 0.1f, 0.8f, 0.1f },
			}, 2);
	}
	
	state->xoffset -= state->tab_size;
	state->current_pos[0] -= state->tab_size;
}

static void
UDebugUI_PushVerticalSpacing(UDebugUI_State* state, float32 spacing)
{
	state->current_pos[1] += spacing * state->scale[1];
}

static void
UDebugUI_End(UDebugUI_State* state)
{
	*state->deferred_background = (E_RectBatchElem) {
		.pos = { state->start_pos[0], state->start_pos[1] },
		.tex_kind = 0,
		.scaling[0][0] = state->max_width + 4.0f,
		.scaling[1][1] = state->current_pos[1] - state->start_pos[1] + 4.0f,
		.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
		.color = { 0.05f, 0.05f, 0.05f, 1.0f },
	};
	
	Mem_Zero(state, sizeof(*state));
}

#endif //UTIL_DEBUGUI_H
