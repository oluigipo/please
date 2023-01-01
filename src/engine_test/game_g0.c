static Game_Data* game;
static Engine_Data* engine;

struct Game_Data
{
	Render_Texture2D texture;
};

static void
Game_Init(void)
{
	game = engine->game = Arena_PushStruct(engine->persistent_arena, Game_Data);
	
	
}

static void
Game_UpdateAndRender(void)
{
	if (engine->platform->window_should_close || Engine_IsPressed(engine->input->keyboard, Engine_KeyboardKey_Escape))
		engine->running = false;
	
	Render_ClearColor(engine, (vec4) { 0.2f, 0.0f, 0.4f, 1.0f });
	
	Engine_FinishFrame();
}

API void
Game_Main(Engine_Data* data)
{
	engine = data;
	game = data->game;
	
	if (!game)
		Game_Init();
	
	for Arena_TempScope(data->scratch_arena)
		Game_UpdateAndRender();
}
