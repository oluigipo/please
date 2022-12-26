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
