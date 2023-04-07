struct G_SnakeTail
{
	int32 x;
	int32 y;
}
typedef G_SnakeTail;

struct G_SnakeState
{
	float32 cell_width;
	float32 cell_height;
	int32 grid_width;
	int32 grid_height;
	uint32 tail_size;
	uint32 tail_cap;
	G_SnakeTail* tail;
	
	int32 apple_x;
	int32 apple_y;
	
	int32 direction;
	int32 previous_direction;
	float32 period;
	float32 time_scale;
};

static void
G_SnakeInit(void)
{
	Trace();
	
	game->snake = Arena_PushStructInit(engine->persistent_arena, G_SnakeState, {
		.cell_width = 24.0f,
		.cell_height = 24.0f,
		.grid_width = 30,
		.grid_height = 20,
		.tail_size = 0,
		.tail_cap = 600,
		.tail = Arena_PushArray(engine->persistent_arena, G_SnakeTail, 600),
		
		.apple_x = URng_Int32Range(&game->rng, 0, 30),
		.apple_y = URng_Int32Range(&game->rng, 0, 20),
		
		.direction = 0,
		.previous_direction = 0,
		.period = 0,
		.time_scale = 1.0f,
	});
	
	for (int32 i = 0; i < 5; ++i)
	{
		++game->snake->tail_size;
		
		game->snake->tail[i].x = 15;
		game->snake->tail[i].y = 10;
	}
}

static void
G_SnakeUpdateAndRender(void)
{
	Trace();
	
	G_SnakeState* s = game->snake;
	s->period += engine->delta_time * s->time_scale;
	s->time_scale = glm_max(s->time_scale - 0.1f, 1.0f);
	
	//- NOTE(ljre): Update
	if (OS_IsPressed(engine->os->keyboard, OS_KeyboardKey_Left) && s->previous_direction != 0)
		s->direction = 2;
	else if (OS_IsPressed(engine->os->keyboard, OS_KeyboardKey_Right) && s->previous_direction != 2)
		s->direction = 0;
	else if (OS_IsPressed(engine->os->keyboard, OS_KeyboardKey_Up) && s->previous_direction != 3)
		s->direction = 1;
	else if (OS_IsPressed(engine->os->keyboard, OS_KeyboardKey_Down) && s->previous_direction != 1)
		s->direction = 3;
	
	if (s->period >= 0.1f)
	{
		s->period -= 0.1f;
		s->previous_direction = s->direction;
		
		int32 move_x = 0;
		int32 move_y = 0;
		
		switch (s->direction)
		{
			case 0: move_x =  1; move_y =  0; break;
			case 1: move_x =  0; move_y = -1; break;
			case 2: move_x = -1; move_y =  0; break;
			case 3: move_x =  0; move_y =  1; break;
			default: SafeAssert(false); break;
		}
		
		for (int32 i = s->tail_size-1; i >= 1; --i)
			s->tail[i] = s->tail[i-1];
		
		s->tail[0].x += move_x;
		s->tail[0].y += move_y;
		
		s->tail[0].x = (s->tail[0].x % s->grid_width  + s->grid_width ) % s->grid_width ;
		s->tail[0].y = (s->tail[0].y % s->grid_height + s->grid_height) % s->grid_height;
		
		for (int32 i = 1; i < s->tail_size; ++i)
		{
			if (s->tail[i].x == s->tail[0].x && s->tail[i].y == s->tail[0].y)
			{
				OS_DebugMessageBox("Game over.");
				game->state = G_GlobalState_MainMenu;
				break;
			}
		}
		
		if (s->tail[0].x == s->apple_x && s->tail[0].y == s->apple_y)
		{
			s->tail[s->tail_size] = s->tail[s->tail_size-1];
			++s->tail_size;
			
			if (s->tail_size >= s->tail_cap)
			{
				OS_DebugMessageBox("You win.");
				game->state = G_GlobalState_MainMenu;
			}
			
			s->apple_x = URng_Int32Range(&game->rng, 0, 30);
			s->apple_y = URng_Int32Range(&game->rng, 0, 20);
			s->time_scale = 3.0f;
		}
	}
	
	//- NOTE(ljre): Draw
	const float32 grid_base_x = (engine->os->window.width - s->grid_width * s->cell_width) * 0.5f;
	const float32 grid_base_y = (engine->os->window.height - s->grid_height * s->cell_height) * 0.5f;
	
	E_RectBatch batch = {
		.arena = engine->frame_arena,
		.textures[0] = NULL,
		.textures[1] = &game->font.texture,
		.count = 0,
		.elements = Arena_EndAligned(engine->frame_arena, alignof(E_RectBatchElem)),
	};
	
	E_PushRect(&batch, &(E_RectBatchElem) {
		.pos = {
			grid_base_x,
			grid_base_y,
		},
		.scaling[0][0] = s->cell_width * s->grid_width,
		.scaling[1][1] = s->cell_height * s->grid_height,
		.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
		.color = { [3] = 0.3f },
	});
	
	vec3 col1 = { 0.5f, 0.9f, 0.5f };
	vec3 col2 = { 0.5f, 0.5f, 0.9f };
	
	E_PushRect(&batch, &(E_RectBatchElem) {
		.pos = {
			grid_base_x + s->tail[0].x * s->cell_width + 1,
			grid_base_y + s->tail[0].y * s->cell_height + 1,
		},
		.tex_kind = 3,
		.scaling[0][0] = s->cell_width - 2,
		.scaling[1][1] = s->cell_height - 2,
		.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
		.color = { col1[0], col1[1], col1[2], 1.0f },
	});
	
	for (int32 i = 1; i < s->tail_size; ++i)
	{
		float32 scale = (float32)i / (s->tail_size-1);
		float32 scale2 = 1.0f - (1.0f-scale)*(1.0f-scale);
		
		E_RectBatchElem elem = {
			.pos = {
				grid_base_x + s->tail[i].x * s->cell_width + 2*glm_lerp(1.0f, 1.5f, scale2),
				grid_base_y + s->tail[i].y * s->cell_height + 2*glm_lerp(1.0f, 1.5f, scale2),
			},
			.tex_kind = 3,
			.scaling[0][0] = s->cell_width  - 4*glm_lerp(1.0f, 1.5f, scale2),
			.scaling[1][1] = s->cell_height - 4*glm_lerp(1.0f, 1.5f, scale2),
			.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
			.color = { [3] = 1.0f },
		};
		
		glm_vec3_lerp(col1, col2, scale, elem.color);
		E_PushRect(&batch, &elem);
	}
	
	E_PushRect(&batch, &(E_RectBatchElem) {
		.pos = {
			grid_base_x + s->apple_x * s->cell_width + 2,
			grid_base_y + s->apple_y * s->cell_height + 2,
		},
		.tex_kind = 3,
		.scaling[0][0] = s->cell_width - 4,
		.scaling[1][1] = s->cell_height - 4,
		.texcoords = { 0, 0, INT16_MAX, INT16_MAX },
		.color = { 0.9f, 0.3f, 0.3f, 1.0f },
	});
	
	if (s->tail_size >= 12)
	{
		E_PushText(&batch, &game->font, Str("yes, this is intentional"), vec2(grid_base_x, grid_base_y - 96.0f), vec2(1.0f, 1.0f), GLM_VEC4_ONE);
	}
	
	uint8 gibberish[20];
	int32 gibberish_size = (int32)roundf(sizeof(gibberish) / 2.0f * glm_min(fabsf(s->time_scale-1.0f), 2.0f));
	
	for (int32 i = 0; i < gibberish_size; ++i)
	{
		static const char buffer[] =
			"!@#$%&*()_+-=1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
			"{}[]ªº~^´`éèÉÈÁà";
		int32 chosen = URng_Int32Range(&game->rng, 0, sizeof(buffer));
		gibberish[i] = buffer[chosen];
	}
	
	String str = String_PrintfLocal(64, "%.*syou are %um long", gibberish_size, gibberish, s->tail_size);
	E_PushText(&batch, &game->font, str, vec2(grid_base_x, grid_base_y - 48.0f), vec2(1.0f, 1.0f), GLM_VEC4_ONE);
	
	E_DrawRectBatch(&batch, NULL);
	
	//- NOTE(ljre): After update
	if (game->state != G_GlobalState_Snake)
	{
		// Cleanup. Nothing needed for now.
	}
}
