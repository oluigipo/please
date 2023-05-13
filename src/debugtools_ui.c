API DBG_UIState
DBG_UIBegin(E_GlobalData* engine, E_RectBatch* batch, E_Font* font, vec2 pos, vec2 scale)
{
	DBG_UIState state = {
		.engine = engine,
		.batch = batch,
		.font = font,
		.deferred_background = ArenaPushStruct(batch->arena, E_RectBatchElem),
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

API bool
DBG_UIPushFoldable(DBG_UIState* state, String text, bool* is_unfolded)
{
	const float32 dot_size = 32.0f * glm_min(state->scale[0], state->scale[1]);
	const float32 dot_pad = 4.0f * glm_min(state->scale[0], state->scale[1]);
	const float32 dot_color = *is_unfolded ? 0.8f : 0.1f;
	
	vec2 size = { 0 };
	E_CalcTextSize(state->font, text, state->scale, &size);
	state->max_width = glm_max(size[0] + state->xoffset + dot_size, state->max_width);
	
	vec2 mouse;
	glm_vec2_copy(state->engine->os->mouse.pos, mouse);
	if (OS_IsPressed(state->engine->os->mouse, OS_MouseButton_Left) &&
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
		.tex_kind = 1,
		.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
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

API void
DBG_UIPopFoldable(DBG_UIState* state)
{
	state->xoffset -= state->tab_size;
	state->current_pos[0] -= state->tab_size;
}

API void
DBG_UIPushTextF(DBG_UIState* state, const char* fmt, ...)
{
	for ArenaTempScope(state->engine->scratch_arena)
	{
		va_list args;
		va_start(args, fmt);
		String text = ArenaVPrintf(state->engine->scratch_arena, fmt, args);
		va_end(args);
		
		vec2 size = { 0 };
		E_CalcTextSize(state->font, text, state->scale, &size);
		state->max_width = glm_max(size[0] + state->xoffset, state->max_width);
		
		E_PushText(state->batch, state->font, text, state->current_pos, state->scale, GLM_VEC4_ONE);
		state->current_pos[1] += size[1] + state->elems_spacing;
	}
}

API bool
DBG_UIPushButton(DBG_UIState* state, String text)
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
	glm_vec2_copy(state->engine->os->mouse.pos, mouse);
	bool is_over = (mouse[0] >= bbox[0] && bbox[2] >= mouse[0] && mouse[1] >= bbox[1] && bbox[3] >= mouse[1]);
	bool result = is_over && OS_IsReleased(state->engine->os->mouse, OS_MouseButton_Left);
	
	float32 color = is_over && OS_IsDown(state->engine->os->mouse, OS_MouseButton_Left) ? 0.3f : 0.2f;
	
	E_PushRect(state->batch, &(E_RectBatchElem) {
		.pos = { bbox[0], bbox[1] },
		.tex_kind = 1,
		.scaling[0][0] = width + padding,
		.scaling[1][1] = height,
		.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
		.color = { color, color, color, 1.0f },
	});
	
	E_PushText(state->batch, state->font, text, vec2(state->current_pos[0] + padding, state->current_pos[1] + padding*0.5f), state->scale, GLM_VEC4_ONE);
	
	state->current_pos[1] += height + state->elems_spacing;
	
	return result;
}

API void
DBG_UIPushSlider(DBG_UIState* state, float32 min, float32 max, float32* value)
{
	// TODO
}

API void
DBG_UIPushProgressBar(DBG_UIState* state, float32 width, float32* ts, vec3* colors, int32 bar_count)
{
	width *= state->scale[0];
	const float32 height = 8.0f * state->scale[1];
	
	state->max_width = glm_max(width + state->xoffset, state->max_width);
	
	E_PushRect(state->batch, &(E_RectBatchElem) {
		.pos = { state->current_pos[0], state->current_pos[1] },
		.tex_kind = 1,
		.scaling[0][0] = width,
		.scaling[1][1] = height,
		.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
		.color = { 0.2f, 0.2f, 0.2f, 1.0f },
	});
	
	for (int32 i = 0; i < bar_count; ++i)
	{
		E_PushRect(state->batch, &(E_RectBatchElem) {
			.pos = { state->current_pos[0], state->current_pos[1] },
			.tex_kind = 1,
			.scaling[0][0] = width * ts[i],
			.scaling[1][1] = height,
			.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
			.color = { colors[i][0], colors[i][1], colors[i][2], 1.0f },
		});
	}
	
	state->current_pos[1] += height + state->elems_spacing;
}

API char
DBG_UIChooseMemoryMetric_(uintsize count, float32* out_flt)
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

API void
DBG_UIPushArenaInfo(DBG_UIState* state, Arena* arena, String name)
{
	float32 f_offset, f_peak, f_commited, f_reserved;
	
	char me_offset = DBG_UIChooseMemoryMetric_(arena->offset, &f_offset);
	char me_peak = DBG_UIChooseMemoryMetric_(arena->peak, &f_peak);
	char me_commited = DBG_UIChooseMemoryMetric_(arena->commited, &f_commited);
	char me_reserved = DBG_UIChooseMemoryMetric_(arena->reserved, &f_reserved);
	
	DBG_UIPushTextF(state, "Arena '%S':", name);
	state->xoffset += state->tab_size;
	state->current_pos[0] += state->tab_size;
	
	if (arena->page_size != 0 && arena->page_size != arena->reserved)
	{
		DBG_UIPushTextF(state, "Offset: %.2f%c current, %.2f%c peak", f_offset, me_offset, f_peak, me_peak);
		DBG_UIPushProgressBar(state, 300.0f,
		(float32[]) {
				(float32)arena->peak / (float32)arena->commited,
				(float32)arena->offset / (float32)arena->commited,
			},
		(vec3[]) {
				{ 0.8f, 0.8f, 0.1f },
				{ 0.1f, 0.8f, 0.1f },
			}, 2);
		
		DBG_UIPushTextF(state, "Virtual: %.2f%c commited, %.2f%c reserved", f_commited, me_commited, f_reserved, me_reserved);
		DBG_UIPushProgressBar(state, 300.0f,
		(float32[]) {
				(float32)arena->commited / (float32)arena->reserved,
			},
		(vec3[]) {
				{ 0.1f, 0.8f, 0.1f },
			}, 1);
	}
	else
	{
		DBG_UIPushTextF(state, "Offset: %.2f%c current, %.2f%c peak\nMax: %.2f%c", f_offset, me_offset, f_peak, me_peak, f_commited, me_commited);
		DBG_UIPushProgressBar(state, 300.0f,
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

API void
DBG_UIPushVerticalSpacing(DBG_UIState* state, float32 spacing)
{
	state->current_pos[1] += spacing * state->scale[1];
}

API String
DBG_UIPushTextField(DBG_UIState* state, uint8* buffer, intsize buffer_cap, intsize* buffer_size, bool* is_selected, float32 min_width)
{
	String text = StrMake(*buffer_size, buffer);
	vec2 text_size;
	E_CalcTextSize(state->font, (text.size == 0 ? Str("!") : text), state->scale, &text_size);
	text_size[0] = glm_max(text_size[0], min_width);
	
	const float32 padding = 4.0f * glm_min(state->scale[0], state->scale[1]);
	vec4 bbox = {
		state->current_pos[0],
		state->current_pos[1],
		state->current_pos[0] + text_size[0] + padding*2,
		state->current_pos[1] + text_size[1] + padding*2,
	};
	
	vec2 mouse;
	glm_vec2_copy(state->engine->os->mouse.pos, mouse);
	
	bool is_over = (mouse[0] >= bbox[0] && bbox[2] >= mouse[0] && mouse[1] >= bbox[1] && bbox[3] >= mouse[1]);
	bool is_pressed = OS_IsPressed(state->engine->os->mouse, OS_MouseButton_Left);
	
	if (is_pressed)
		*is_selected = is_over;
	
	if (*is_selected)
	{
		intsize size = *buffer_size;
		
		for (intsize i = 0; i < state->engine->os->text_codepoints_count; ++i)
		{
			uint32 codepoint = state->engine->os->text_codepoints[i];
			
			if (codepoint == '\b')
			{
				if (size > 0)
				{
					if (buffer[size-1] & 0x80)
						while (size > 0 && (buffer[--size] & 0xb0) != 0x80);
					else
						--size;
				}
			}
			else if (codepoint == 0x7f/*DEL*/)
			{
				bool beginning = (buffer[size-1] == ' ');
				
				while (size > 0 && (buffer[size-1] == ' ') == beginning)
					--size;
			}
			else
			{
				if (codepoint == '\r')
					codepoint = '\n';
				
				int32 required_size = StringEncodedCodepointSize(codepoint);
				
				if (size + required_size <= buffer_cap)
				{
					StringEncode(buffer + size, required_size, codepoint);
					size += required_size;
				}
				else
					break;
			}
		}
		
		*buffer_size = size;
		
		text = StrMake(size, buffer);
		E_CalcTextSize(state->font, (text.size == 0 ? Str("!") : text), state->scale, &text_size);
		text_size[0] = glm_max(text_size[0], min_width);
		
		bbox[2] = state->current_pos[0] + text_size[0] + padding*2;
		bbox[3] = state->current_pos[1] + text_size[1] + padding*2;
	}
	
	E_RectBatchElem elem = {
		.pos = { bbox[0], bbox[1] },
		.tex_kind = 1,
		.scaling[0][0] = bbox[2] - bbox[0],
		.scaling[1][1] = bbox[3] - bbox[1],
		.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
		.color = { 0.2f, 0.2f, 0.2f, 1.0f },
	};
	
	if (*is_selected)
		glm_vec3_copy(vec3(0.3f, 0.3f, 0.3f), elem.color);
	
	E_PushRect(state->batch, &elem);
	E_PushText(state->batch, state->font, text, vec2(bbox[0] + padding, bbox[1] + padding), state->scale, GLM_VEC4_ONE);
	
	state->max_width = glm_max(bbox[2] - state->start_pos[0], state->max_width);
	state->current_pos[1] += (bbox[3] - bbox[1]) + state->elems_spacing;
	
	return text;
}

API void
DBG_UIEnd(DBG_UIState* state)
{
	*state->deferred_background = (E_RectBatchElem) {
		.pos = { state->start_pos[0], state->start_pos[1] },
		.tex_kind = 0,
		.scaling[0][0] = state->max_width + 4.0f,
		.scaling[1][1] = state->current_pos[1] - state->start_pos[1] + 4.0f,
		.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
		.color = { 0.05f, 0.05f, 0.05f, 1.0f },
	};
	
	MemoryZero(state, sizeof(*state));
}
