/*
* [ Quick game summary ]
* You run, you kill -- Hotline miami style
* EDG32
*/

#define Game_MAX_PLAYING_AUDIOS 32

enum Game_State
{
	Game_State_Menu,
	Game_State_Gameplay,
}
typedef Game_State;

struct Game_GameplayData
{
	vec3 player_pos;
	vec2 player_speed;
	
	vec3 camera_pos;
	vec2 camera_size;
	float32 camera_zoom;
	
	vec3 texture_cell;
	vec3 texture_cell_uv;
	
	vec4 walls[1];
}
typedef Game_GameplayData;

struct Game_Data
{
	// NOTE(ljre): Assets
	Asset_Font font;
	Asset_Texture sprites;
	
	// NOTE(ljre): Audio
	Engine_PlayingAudio playing_audios[Game_MAX_PLAYING_AUDIOS];
	int32 playing_audio_count;
	
	// NOTE(ljre): Options
	float32 master_volume;
	int32 controller_index;
	
	// NOTE(ljre): Game state
	Game_State state;
	
	union
	{
		Game_GameplayData gameplay;
	};
};

internal void
PushAudio(Engine_Data* g, const Asset_SoundBuffer* sound)
{
	if (g->game->playing_audio_count < Game_MAX_PLAYING_AUDIOS)
	{
		int32 index = g->game->playing_audio_count++;
		Engine_PlayingAudio* playing = &g->game->playing_audios[index];
		
		playing->sound = sound;
		playing->frame_index = -1;
		playing->loop = false;
		playing->volume = 1.0f;
		playing->speed = 1.0f;
	}
}

internal void
LoadEssentialAssets(Engine_Data* g)
{
	if (!Render_LoadFontFromFile(Str("./assets/FalstinRegular-XOr2.ttf"), &g->game->font))
		Platform_ExitWithErrorMessage(Str("Could not load font './assets/FalstinRegular-XOr2.ttf'."));
	if (!Render_LoadTextureFromFile(Str("./assets/sprites.png"), &g->game->sprites))
		Platform_ExitWithErrorMessage(Str("Could not sprites from file './assets/sprites.png'."));
}

internal void
CalcUvs(vec4 out, vec2 cell_size, vec2 coord)
{
	vec4 result = { 0.0f };
	vec2 tmp;
	
	glm_vec2_mul(cell_size, coord, tmp); // tmp = cell_size * coord
	glm_vec2_add(tmp, result, result);   // result[0:2] *= tmp
	
	glm_vec2_copy(result, result+2);     // result[2:4] = result[0:2]
	glm_vec2_add(cell_size, result+2, result+2); // result[2:4] += cell_size
	
	glm_vec4_copy(result, out);
}

// NOTE(ljre): Returns -1 if no collision.
internal int32
AmCollidingWithSomeWall(vec2 self, vec2 size, vec2 speed, vec4* walls, int32 wall_count)
{
	for (int32 i = 0; i < wall_count; ++i)
	{
		float32* wallpos = walls[i];
		float32* wallsize = wallpos + 2;
		
		if (self[0]+size[0] > wallpos[0] && wallpos[0]+wallsize[0] > self[0] &&
			self[1]+size[1] > wallpos[1] && wallpos[1]+wallsize[1] > self[1])
		{
			return i;
		}
	}
	
	return -1;
}

internal void
Game_JamPlay(Engine_Data* g, bool32 need_init)
{
	const float32 player_speed_const = 3.0f;
	Game_GameplayData* gm = &g->game->gameplay;
	
	if (need_init)
	{
		*gm = (Game_GameplayData) {
			.player_pos = { 0.0f, 0.0f },
			.player_speed = { 0.0f, 0.0f },
			
			.camera_pos = { 0.0f, 0.0f },
			.camera_size = { (float32)Platform_WindowWidth(), (float32)Platform_WindowHeight() },
			.camera_zoom = 6.0f,
			
			.texture_cell = { 32.0f, 32.0f },
			
			.texture_cell_uv = {
				1.0f / (float32)g->game->sprites.width * 32.0f,
				1.0f / (float32)g->game->sprites.height * 32.0f,
				1.0f, // NOTE(ljre): For convenience when scaling.
			},
			
			.walls = {
				{ 200.0f, 200.0f, 32.0f, 64.0f },
			},
		};
	}
	
	void* temp_save = Arena_End(g->temp_arena);
	
	//~ NOTE(ljre): Update
	if (Platform_WindowShouldClose())
	{
		g->running = false;
	}
	
	// NOTE(ljre): Mouse pos in game
	vec2 mouse_pos;
	{
		Input_Mouse mouse;
		Input_GetMouse(&mouse);
		
		glm_vec2_negate_to(gm->camera_size, mouse_pos);
		glm_vec2_scale(mouse_pos, 0.5f, mouse_pos);
		
		glm_vec2_add(mouse_pos, gm->camera_pos, mouse_pos);
		glm_vec2_add(mouse_pos, mouse.pos, mouse_pos);
	}
	
	// NOTE(ljre): Player movement
	{
		int32 i_dir_x = Input_KeyboardIsDown('D') - Input_KeyboardIsDown('A');
		int32 i_dir_y = Input_KeyboardIsDown('S') - Input_KeyboardIsDown('W');
		
		vec2 target_speed = { (float32)i_dir_x, (float32)i_dir_y };
		
		if (i_dir_x && i_dir_y)
			glm_vec2_scale(target_speed, 0.707f, target_speed); // NOTE(ljre): 0.707 ~= sqrt(2)/2
		
		glm_vec2_scale(target_speed, player_speed_const, target_speed);
		glm_vec2_lerp(gm->player_speed, target_speed, 0.4f, gm->player_speed);
		
		glm_vec2_add(gm->player_pos, gm->player_speed, gm->player_pos);
	}
	
	//~ NOTE(ljre): Render
	Render_ClearBackground(1.0f, 0.9f, 0.5f, 1.0f);
	
	// NOTE(ljre): Camera
	mat4 view;
	{
		Render_Camera camera = {
			.zoom = gm->camera_zoom,
		};
		
		glm_vec2_copy(gm->camera_size, camera.size);
		
		vec2 target_camera_pos;
		glm_vec2_lerp(gm->player_pos, mouse_pos, 0.1f, target_camera_pos);
		
		glm_vec2_lerp(gm->camera_pos, target_camera_pos, 0.3f, gm->camera_pos);
		glm_vec2_copy(gm->camera_pos, camera.pos);
		
		Render_CalcViewMatrix2D(&camera, view);
	}
	
	//- NOTE(ljre): Layer
	Render_Layer2D layer = {
		.texture = &g->game->sprites,
		
		.sprite_count = 1 + ArrayLength(gm->walls),
		.sprites = Arena_Push(g->temp_arena, sizeof(layer.sprites[0]) * (1 + ArrayLength(gm->walls))),
	};
	
	// NOTE(ljre): Player Sprite
	{
		Render_Sprite2D* spr_player = &layer.sprites[0];
		
		glm_mat4_identity(spr_player->transform);
		glm_translate(spr_player->transform, gm->player_pos);
		glm_scale(spr_player->transform, gm->texture_cell);
		glm_translate(spr_player->transform, (vec3) { -0.5f, -0.5f });
		
		CalcUvs(spr_player->texcoords, gm->texture_cell_uv, (vec2) { 0, 0 });
		
		glm_vec4_one(spr_player->color);
	}
	
	// NOTE(ljre): Walls
	for (int32 i = 0; i < ArrayLength(gm->walls); ++i)
	{
		Render_Sprite2D* spr = &layer.sprites[1+i];
		float32* wall = gm->walls[i];
		
		glm_mat4_identity(spr->transform);
		glm_translate(spr->transform, (vec3) { wall[0], wall[1] });
		glm_scale(spr->transform, (vec3) { wall[2], wall[3], 1.0f });
		
		CalcUvs(spr->texcoords, gm->texture_cell_uv, (vec2) { 1, 0 });
		
		glm_vec4_one(spr->color);
		glm_vec3_scale(spr->color, 0.25f, spr->color);
	}
	
	Render_DrawLayer2D(&layer, view);
	
	//~ NOTE(ljre): Done
	Engine_PlayAudios(g->game->playing_audios, &g->game->playing_audio_count, g->game->master_volume);
	Engine_FinishFrame();
	
	Arena_Pop(g->temp_arena, temp_save);
}

API void
Game_Main(Engine_Data* g)
{
	if (!g->game)
	{
		g->game = Arena_Push(g->persistent_arena, sizeof(*g->game));
		g->game->master_volume = 0.25f;
		LoadEssentialAssets(g);
	}
	
	switch (g->game->state)
	{
		case Game_State_Menu:
		{
			if (Platform_WindowShouldClose())
				g->running = false;
			
			//~ NOTE(ljre): Update
			Input_Mouse mouse;
			Input_GetMouse(&mouse);
			
			if (Input_IsPressed(mouse, Input_MouseButton_Left))
			{
				g->game->state = Game_State_Gameplay;
				Game_JamPlay(g, true);
				break;
			}
			
			//~ NOTE(ljre): Draw
			Render_ClearBackground(0.0f, 0.0f, 0.0f, 1.0f);
			
			Render_Begin2D();
			{
				vec3 pos = {
					(float32)Platform_WindowWidth() / 2.0f,
					(float32)Platform_WindowHeight() / 2.0f,
				};
				
				vec3 align = { -0.5f, -0.5f };
				
				Render_DrawText(&g->game->font, Str("Click anywhere to start!"), pos, 42.0f, GLM_VEC4_ONE, align);
			}
			
			Engine_FinishFrame();
		} break;
		
		case Game_State_Gameplay:
		{
			Game_JamPlay(g, false);
		} break;
	}
}
