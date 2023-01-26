#include "api_engine.h"
#include "util_random.h"

static G_GlobalData* game;
static E_GlobalData* engine;

struct G_SnakeState typedef G_SnakeState;

enum G_GlobalState
{
	G_GlobalState_MainMenu,
	G_GlobalState_Snake,
}
typedef G_GlobalState;

struct G_GlobalData
{
	G_GlobalState state;
	G_GlobalState previous_state;
	E_Font font;
	Arena_Savepoint persistent_arena_save;
	URng_State rng;
	
	union
	{
		G_SnakeState* snake;
	};
};

//~ NOTE(ljre): Snake game implementation
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
	
	game->snake = Arena_PushStruct(engine->persistent_arena, G_SnakeState);
	
	*game->snake = (G_SnakeState) {
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
	};
	
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
	if (OS_IsPressed(engine->input->keyboard, OS_KeyboardKey_Left) && s->previous_direction != 0)
		s->direction = 2;
	else if (OS_IsPressed(engine->input->keyboard, OS_KeyboardKey_Right) && s->previous_direction != 2)
		s->direction = 0;
	else if (OS_IsPressed(engine->input->keyboard, OS_KeyboardKey_Up) && s->previous_direction != 3)
		s->direction = 1;
	else if (OS_IsPressed(engine->input->keyboard, OS_KeyboardKey_Down) && s->previous_direction != 1)
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
	const float32 grid_base_x = (engine->window_state->width - s->grid_width * s->cell_width) * 0.5f;
	const float32 grid_base_y = (engine->window_state->height - s->grid_height * s->cell_height) * 0.5f;
	
	E_RectBatchElem* elem = NULL;
	E_RectBatch batch = {
		.textures[0] = NULL,
		.textures[1] = &game->font.texture,
		.count = 0,
		.elements = Arena_EndAligned(engine->frame_arena, alignof(E_RectBatchElem)),
	};
	
	++batch.count;
	elem = Arena_PushStruct(engine->frame_arena, E_RectBatchElem);
	*elem = (E_RectBatchElem) {
		.pos = {
			grid_base_x,
			grid_base_y,
		},
		.scaling[0][0] = s->cell_width * s->grid_width,
		.scaling[1][1] = s->cell_height * s->grid_height,
		.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
		.color = { [3] = 0.3f },
	};
	
	vec3 col1 = { 0.5f, 0.9f, 0.5f };
	vec3 col2 = { 0.5f, 0.5f, 0.9f };
	
	++batch.count;
	elem = Arena_PushStruct(engine->frame_arena, E_RectBatchElem);
	*elem = (E_RectBatchElem) {
		.pos = {
			grid_base_x + s->tail[0].x * s->cell_width + 1,
			grid_base_y + s->tail[0].y * s->cell_height + 1,
		},
		.scaling[0][0] = s->cell_width - 2,
		.scaling[1][1] = s->cell_height - 2,
		.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
		.color = { col1[0], col1[1], col1[2], 1.0f },
	};
	
	for (int32 i = 1; i < s->tail_size; ++i)
	{
		float32 scale = (float32)i / (s->tail_size-1);
		float32 scale2 = 1.0f - (1.0f-scale)*(1.0f-scale);
		
		++batch.count;
		elem = Arena_PushStruct(engine->frame_arena, E_RectBatchElem);
		*elem = (E_RectBatchElem) {
			.pos = {
				grid_base_x + s->tail[i].x * s->cell_width + 2*glm_lerp(1.0f, 1.5f, scale2),
				grid_base_y + s->tail[i].y * s->cell_height + 2*glm_lerp(1.0f, 1.5f, scale2),
			},
			.scaling[0][0] = s->cell_width  - 4*glm_lerp(1.0f, 1.5f, scale2),
			.scaling[1][1] = s->cell_height - 4*glm_lerp(1.0f, 1.5f, scale2),
			.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
			.color = { [3] = 1.0f },
		};
		
		glm_vec3_lerp(col1, col2, scale, elem->color);
	}
	
	++batch.count;
	elem = Arena_PushStruct(engine->frame_arena, E_RectBatchElem);
	*elem = (E_RectBatchElem) {
		.pos = {
			grid_base_x + s->apple_x * s->cell_width + 2,
			grid_base_y + s->apple_y * s->cell_height + 2,
		},
		.scaling[0][0] = s->cell_width - 4,
		.scaling[1][1] = s->cell_height - 4,
		.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
		.color = { 0.9f, 0.3f, 0.3f, 1.0f },
	};
	
	if (s->tail_size >= 12)
	{
		E_PushTextToRectBatch(&batch, engine->frame_arena, &game->font, Str("yes, this is intentional"), vec2(grid_base_x, grid_base_y - 96.0f), vec2(1.0f, 1.0f), GLM_VEC4_ONE);
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
	E_PushTextToRectBatch(&batch, engine->frame_arena, &game->font, str, vec2(grid_base_x, grid_base_y - 48.0f), vec2(1.0f, 1.0f), GLM_VEC4_ONE);
	
	E_DrawRectBatch(&batch, NULL);
	
	//- NOTE(ljre): After update
	if (game->state != G_GlobalState_Snake)
	{
		// Cleanup. Nothing needed for now.
	}
}

//~ NOTE(ljre): Main menu implementation
static void
G_Init(void)
{
	Trace();
	
	game = engine->game = Arena_PushStruct(engine->persistent_arena, G_GlobalData);
	game->previous_state = game->state = G_GlobalState_MainMenu;
	
	URng_Init(&game->rng, OS_CurrentPosixTime());
	
	E_FontDesc desc = {
		.arena = engine->persistent_arena,
		.ttf = { 0 },
		
		.char_height = 32.0f,
		.prebake_ranges = {
			{ 0x21, 0x7E+1 },
			{ 0xA1, 0xFF+1 },
		},
	};
	
	SafeAssert(OS_ReadEntireFile(Str("C:/Windows/Fonts/Arial.ttf"), engine->persistent_arena, (void**)&desc.ttf.data, &desc.ttf.size));
	SafeAssert(E_MakeFont(&desc, &game->font));
	
	game->persistent_arena_save = Arena_Save(engine->persistent_arena);
}

static bool
G_MenuButton(E_RectBatch* batch, float32* inout_y, String text)
{
	Trace();
	
	const float32 button_width = 200.0f;
	const float32 button_height = 60.0f;
	const float32 button_x = (engine->window_state->width - button_width) / 2;
	const float32 mouse_x = engine->input->mouse.pos[0];
	const float32 mouse_y = engine->input->mouse.pos[1];
	
	float32 button_y = *inout_y;
	
	bool is_mouse_pressing = OS_IsDown(engine->input->mouse, OS_MouseButton_Left);
	bool is_mouse_over = true
		&& (mouse_x >= button_x && button_x + button_width  >= mouse_x)
		&& (mouse_y >= button_y && button_y + button_height >= mouse_y);
	
	vec4 color = { 0.3f, 0.3f, 0.3f, 1.0f };
	
	if (is_mouse_over)
	{
		if (is_mouse_pressing)
			color[0] = color[1] = color[2] = 0.2f;
		else
			color[0] = color[1] = color[2] = 0.4f;
	}
	
	++batch->count;
	E_RectBatchElem* elem = Arena_PushStruct(engine->frame_arena, E_RectBatchElem);
	*elem = (E_RectBatchElem) {
		.pos = { button_x, button_y },
		.scaling[0][0] = button_width,
		.scaling[1][1] = button_height,
		.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
		.color = { color[0], color[1], color[2], color[3] },
	};
	
	E_PushTextToRectBatch(batch, engine->frame_arena, &game->font, text, vec2(button_x, button_y + button_height*0.25f), vec2(1.0f, 1.0f), GLM_VEC4_ONE);
	
	button_y += button_height * 1.5f;
	
	*inout_y = button_y;
	return is_mouse_over && OS_IsReleased(engine->input->mouse, OS_MouseButton_Left);
}

static void
G_UpdateAndRender(void)
{
	Trace();
	
	if (engine->window_state->should_close || OS_IsPressed(engine->input->keyboard, OS_KeyboardKey_Escape))
		engine->running = false;
	
	E_DrawClear(0.2f, 0.0f, 0.4f, 1.0f);
	
	if (game->previous_state != game->state)
	{
		switch (game->state)
		{
			case G_GlobalState_MainMenu: Arena_Restore(game->persistent_arena_save); break;
			case G_GlobalState_Snake: G_SnakeInit(); break;
		}
		
		game->previous_state = game->state;
	}
	
	switch (game->state)
	{
		case G_GlobalState_MainMenu:
		{
			float32 ui_y = 100.0f;
			E_RectBatch batch = {
				.textures[0] = NULL,
				.textures[1] = &game->font.texture,
				.count = 0,
				.elements = Arena_EndAligned(engine->frame_arena, alignof(E_RectBatchElem)),
			};
			
			//- Squares
			for (int32 i = 0; i < 100; ++i)
			{
				uint64 random = Hash_IntHash64(i);
				
				float32 time = (float32)OS_GetTimeInSeconds() * 0.25f + (float32)i;
				float32 xoffset = cosf(time * 5.0f) * 100.0f;
				float32 yoffset = sinf(time * 1.4f) * 100.0f;
				
				float32 x = (random & 511) - 256.0f + xoffset;
				float32 y = (random>>9 & 511) - 256.0f + yoffset;
				float32 angle = (random>>18 & 511) / 512.0f * (float32)Math_PI*2 + time;
				float32 size = 32.0f;
				vec3 color = {
					(random>>27 & 255) / 255.0f,
					(random>>35 & 255) / 255.0f,
					(random>>43 & 255) / 255.0f,
				};
				
				++batch.count;
				E_RectBatchElem* elem = Arena_PushStruct(engine->frame_arena, E_RectBatchElem);
				*elem = (E_RectBatchElem) {
					.pos = { engine->window_state->width*0.5f + x, engine->window_state->height*0.5f + y },
					.scaling = {
						{ size*cosf(angle), size*-sinf(angle) },
						{ size*sinf(angle), size* cosf(angle) },
					},
					.tex_index = 0.0f,
					.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
					.color = { color[0], color[1], color[2], 1.0f },
				};
			}
			
			//- Buttons
			if (G_MenuButton(&batch, &ui_y, Str("Play Snake")))
				game->state = G_GlobalState_Snake;
			if (G_MenuButton(&batch, &ui_y, Str("Quit")))
				engine->running = false;
			
			E_DrawRectBatch(&batch, NULL);
		} break;
		
		case G_GlobalState_Snake: G_SnakeUpdateAndRender(); break;
	}
	
	E_FinishFrame();
}

API void
G_Main(E_GlobalData* data)
{
	Trace();
	
	engine = data;
	game = data->game;
	
	if (!game)
		G_Init();
	
	for Arena_TempScope(data->scratch_arena)
		G_UpdateAndRender();
}
