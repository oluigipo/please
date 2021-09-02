internal void*
Game_2DDemoScene(void** shared_data)
{
	Trace("Game_2DDemoScene");
	
	Game_GlobalData* global = *shared_data;
	
	//~ NOTE(ljre): Assets
	Asset_Texture sprites_texture;
	if (!Render_LoadTextureFromFile(Str("./assets/sprites.png"), &sprites_texture))
	{
		Platform_ExitWithErrorMessage(Str("Could not load sprites image :("));
	}
	
	Asset_Texture tileset_texture;
	if (!Render_LoadTextureFromFile(Str("./assets/tileset.png"), &tileset_texture))
	{
		Platform_ExitWithErrorMessage(Str("Could not load tileset image :("));
	}
	
	//~ NOTE(ljre): Scene Setup
	Render_Camera camera = {
		.pos = { 0.0f, 0.0f, 0.0f, },
		.size = {
			(float32)Platform_WindowWidth(),
			(float32)Platform_WindowHeight(),
		},
		.zoom = 2.0f,
		.angle = 0.0f,
	};
	
	//- NOTE(ljre): Tilemap
	uint16 map[8*8] = {
		0, 1, 1, 1, 1, 1, 1, 0,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 1, 0, 0, 1, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 1, 1, 1, 1, 0, 1,
		1, 0, 0, 1, 1, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		0, 1, 1, 1, 1, 1, 1, 0,
	};
	
	Render_Tilemap2D tilemap = {
		.transform = GLM_MAT4_IDENTITY_INIT,
		.color = { 1.0f, 1.0f, 1.0f, 1.0f },
		.cell_uv_size = {
			1.0f / ((float32)tileset_texture.width / 32.0f),
			1.0f / ((float32)tileset_texture.height / 32.0f),
		},
		.width = 8,
		.height = 8,
		.indices = map,
	};
	
	glm_scale(tilemap.transform, (vec3) { 32.0f, 32.0f });
	
	Render_Layer2D tilemap_layer = {
		.texture = &tileset_texture,
		.tilemaps = &tilemap,
		.tilemap_count = 1,
	};
	
	//- NOTE(ljre): Sprites
	Render_Sprite2D sprites[30];
	Render_Layer2D sprite_layer = {
		.texture = &sprites_texture,
		.sprite_count = ArrayLength(sprites),
		.sprites = sprites,
	};
	
	for (int32 i = 0; i < ArrayLength(sprites); ++i)
	{
		Render_Sprite2D* sprite = &sprites[i];
		
		sprite->texcoords[0] = 0.0f;
		sprite->texcoords[1] = 0.0f;
		sprite->texcoords[2] = 1.0f / ((float32)sprites_texture.width / 32.0f);
		sprite->texcoords[3] = 1.0f / ((float32)sprites_texture.height / 32.0f);
		
		sprite->color[0] = 1.0f;
		sprite->color[1] = 1.0f;
		sprite->color[2] = 1.0f;
		sprite->color[3] = 1.0f;
		
		vec2 pos = {
			Engine_RandomF32Range(-500.0f, 500.0f),
			Engine_RandomF32Range(-500.0f, 500.0f),
		};
		
		vec2 scale = { 32.0f, 32.0f };
		float32 angle = Engine_RandomF32Range(0.0f, PI32 * 2.0f);
		
		Render_CalcModelMatrix2D(pos, scale, angle, sprite->transform);
	}
	
	// NOTE(ljre): Game Loop
	while (!Platform_WindowShouldClose())
	{
		Trace("Game Loop");
		void* memory_state = Engine_PushMemoryState();
		
		//~ NOTE(ljre): Update
		{
			if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape))
				break;
		}
		
		//~ NOTE(ljre): Render
		{
			Render_ClearBackground(0.0f, 0.0f, 0.0f, 1.0f);
			
			mat4 view;
			Render_CalcViewMatrix2D(&camera, view);
			
			Render_DrawLayer2D(&tilemap_layer, view);
			Render_DrawLayer2D(&sprite_layer, view);
		}
		
		//~ NOTE(ljre): Finish Frame
		Engine_PopMemoryState(memory_state);
		Engine_FinishFrame();
	}
	
	return NULL;
}

