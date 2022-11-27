static Engine_Data* engine;
static Game_Data* game;

struct Game_Data
{
	Render_Texture2D texture;
};

static void
Game_Init(void)
{
	game = engine->game = Arena_Push(engine->persistent_arena, sizeof(*game));
	
	uintsize size;
	void* data = Platform_ReadEntireFile(Str("assets/base_texture.png"), &size, engine->scratch_arena);
	
	Render_Texture2DDesc desc = {
		.encoded_image = data,
		.encoded_image_size = size,
	};
	
	bool ok = Render_MakeTexture2D(engine, &desc, &game->texture);
	
	Arena_Pop(engine->scratch_arena, data);
	
	Assert(ok);
}

static void
Game_UpdateAndRender(void)
{
	void* saved_arena = Arena_End(engine->scratch_arena);
	
	{
		//~ Update
		if (Engine_IsPressed(engine->input->keyboard, Engine_KeyboardKey_Escape) || engine->platform->window_should_close)
			engine->running = false;
		
		//~ Render
		Render_ClearColor(engine, (vec4) { 0.1f, 0.0f, 0.3f, 1.0f });
		
		Render_Data2DInstance spr = {
			.transform = GLM_MAT4_IDENTITY_INIT,
			.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
			.color = { 1.0f, 1.0f, 1.0f, 1.0f },
		};
		
		glm_scale(spr.transform, (vec3) { (float32)game->texture.width, (float32)game->texture.height, 1.0f, });
		
		Render_Data2D data2d = {
			.texture = &game->texture,
			
			.instances = &spr,
			.instance_count = 1,
		};
		
		Render_Draw2D(engine, &data2d);
	}
	
	Arena_Pop(engine->scratch_arena, saved_arena);
	Engine_FinishFrame();
}

API void
Game_Main(Engine_Data* data)
{
	engine = data;
	game = data->game;
	
	if (!data->game)
		Game_Init();
	
	Game_UpdateAndRender();
}
