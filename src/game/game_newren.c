static Engine_Data* engine;
static Game_Data* game;

struct Game_Data
{
	Render_Texture2D texture;
	Render_Font font;
};

static void
Game_Init(void)
{
	Trace();
	
	game = engine->game = Arena_Push(engine->persistent_arena, sizeof(*game));
	
	// NOTE(ljre): Load texture
	{
		uintsize size;
		void* data = Platform_ReadEntireFile(Str("assets/base_texture.png"), &size, engine->scratch_arena);
		
		Render_Texture2DDesc desc = {
			.encoded_image = data,
			.encoded_image_size = size,
		};
		
		Render_MakeTexture2D(engine, &desc, &game->texture);
		
		Arena_Pop(engine->scratch_arena, data);
	}
	
	// NOTE(ljre): Load font
	{
		//String path = StrInit("assets/FalstinRegular-XOr2.ttf");
		String path = StrInit("c:/windows/fonts/arial.ttf");
		
		uintsize size;
		void* data = Platform_ReadEntireFile(path, &size, engine->scratch_arena);
		Assert(data);
		
		Render_FontDesc desc = {
			.arena = engine->persistent_arena,
			
			.ttf_data = data,
			.ttf_data_size = size,
			
			.char_height = 24.0f * 2,
			.glyph_cache_size = 4,
			
			.mag_linear = true,
			.min_linear = true,
		};
		
		Render_MakeFont(engine, &desc, &game->font);
		
		Arena_Pop(engine->scratch_arena, data);
	}
}

static void
Game_UpdateAndRender(void)
{
	Trace();
	
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
		
		String text = String_PrintfLocal(256, "mouse: (%f, %f)", engine->input->mouse.pos[0], engine->input->mouse.pos[1]);
		Render_BatchText(engine, &game->font, text, GLM_VEC4_ONE, (vec2) { -200.0f, 100.0f }, GLM_VEC2_ZERO, (vec2) { 1.0f, 1.0f }, engine->scratch_arena, &data2d);
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
