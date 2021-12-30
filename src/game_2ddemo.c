struct Game_PlayerState
{
	vec2 pos;
}
typedef Game_PlayerState;

struct Game_GlobalState
{
	// NOTE(ljre): Assets
	Asset_Texture sprites_texture;
	vec2 sprites_texture_texel;
	
	// NOTE(ljre): Camera
	Render_Camera camera;
	vec2 camera_target_pos;
	float32 camera_target_zoom;
	float32 camera_shake;
	
	// NOTE(ljre): Player
	Game_PlayerState player;
}
typedef Game_GlobalState;

internal Arena* global_scratch_arena;

internal void
Game_2DDemoScene(Engine_Data* g)
{
	void* mem_state = Arena_End(g->persistent_arena);
	
	Trace("Game_2DDemoScene");
	//Platform_SetWindow(-1, -1, 640, 480);
	//Platform_CenterWindow();
	//Platform_PollEvents(); // "force" window update
	
	Game_GlobalState* global = Arena_Push(g->persistent_arena, sizeof *global);
	
	//~ NOTE(ljre): Assets
	if (!Render_LoadTextureFromFile(Str("./assets/sprites.png"), &global->sprites_texture))
		Platform_ExitWithErrorMessage(Str("Could not load sprites image :("));
	
	global->sprites_texture_texel[0] = 1.0f / (float32)global->sprites_texture.width;
	global->sprites_texture_texel[1] = 1.0f / (float32)global->sprites_texture.height;
	
	//~ NOTE(ljre): Scene Setup
	global->camera = (Render_Camera) {
		.pos = { 0.0f, 0.0f, 0.0f, },
		.size = {
			(float32)Platform_WindowWidth(),
			(float32)Platform_WindowHeight(),
		},
		.zoom = 4.0f,
		.angle = 0.0f,
	};
	
	// NOTE(ljre): Game Loop
	while (!Platform_WindowShouldClose())
	{
		Trace("Game Loop");
		void* temp_memory_state = Arena_End(g->temp_arena);
		
		//~ NOTE(ljre): Update
		{
			if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape))
				break;
			
			float32 move_x = (float32)(Input_KeyboardIsDown('D') - Input_KeyboardIsDown('A'));
			float32 move_y = (float32)(Input_KeyboardIsDown('S') - Input_KeyboardIsDown('W'));
			
			if (move_x != 0.0f && move_y != 0.0f)
			{
				move_x *= 0.707f;
				move_y *= 0.707f;
			}
			
			global->player.pos[0] += move_x * 5.0f;
			global->player.pos[1] += move_y * 5.0f;
		}
		
		//~ NOTE(ljre): Render
		{
			Render_ClearBackground(0.0f, 0.0f, 0.0f, 1.0f);
			Render_Begin2D();
			
			//- NOTE(ljre): Some UVs
			const vec4 uv_player = {
				0.0f,
				0.0f,
				
				32.0f * global->sprites_texture_texel[0],
				32.0f * global->sprites_texture_texel[1],
			};
			
			//- NOTE(ljre): Process sprites to render
			Render_Sprite2D* sprites = Arena_PushAligned(g->temp_arena, sizeof(*sprites) * 32, 15);
			int32 sprite_count = 0;
			
			// NOTE(ljre): Player
			{
				Render_Sprite2D* spr = &sprites[sprite_count++];
				Game_PlayerState* player = &global->player;
				
				glm_mat4_identity(spr->transform);
				glm_translate(spr->transform, (vec3) { player->pos[0], player->pos[1] });
				glm_scale(spr->transform, (vec3) { 32.0f, 32.0f });
				
				spr->texcoords[0] = uv_player[0];
				spr->texcoords[1] = uv_player[1];
				spr->texcoords[2] = uv_player[2];
				spr->texcoords[3] = uv_player[3];
				
				spr->color[0] = spr->color[1] = spr->color[2] = spr->color[3] = 1.0f;
			}
			
			//- NOTE(ljre): Layer
			Render_Layer2D layer_to_render = {
				.texture = &global->sprites_texture,
				.sprite_count = sprite_count,
				.sprites = sprites,
			};
			
			//- NOTE(ljre): View Matrix & Render
			mat4 view_matrix;
			Render_CalcViewMatrix2D(&global->camera, view_matrix);
			
			Render_DrawLayer2D(&layer_to_render, view_matrix);
		}
		
		//~ NOTE(ljre): Finish Frame
		Arena_Pop(g->temp_arena, temp_memory_state);
		Engine_FinishFrame();
	}
	
	Arena_Pop(g->persistent_arena, mem_state);
	g->current_scene = NULL;
}

