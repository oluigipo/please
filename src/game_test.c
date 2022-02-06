#define Game_MAX_PLAYING_AUDIOS 32

struct Game_Data
{
	// NOTE(ljre): Assets
	Asset_Font font;
	Asset_Texture base_texture;
	
	// NOTE(ljre): Audio
	Engine_PlayingAudio playing_audios[Game_MAX_PLAYING_AUDIOS];
	int32 playing_audio_count;
	
	// NOTE(ljre): Options
	float32 master_volume;
	
	// NOTE(ljre): Gameplay Data
	struct
	{
		vec2 camera_pos;
		vec2 camera_speed;
		float32 camera_zoom;
		float32 camera_target_zoom;
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
CalcUvs(vec4 out, vec2 cell_size, vec2 coord)
{
	vec4 result = { 0.0f };
	vec2 tmp;
	
	glm_vec2_mul(cell_size, coord, tmp); // tmp = cell_size * coord
	glm_vec2_add(tmp, result, result);   // result[0:2] += tmp
	
	glm_vec2_copy(result, result+2);     // result[2:4] = result[0:2]
	glm_vec2_add(cell_size, result+2, result+2); // result[2:4] += cell_size
	
	glm_vec4_copy(result, out);
}

internal void
InitGame(Engine_Data* g)
{
	Game_Data* gm = g->game = Arena_Push(g->persistent_arena, sizeof(*g->game));
	gm->master_volume = 0.25f;
	
	if (!Render_LoadFontFromFile(Str("./assets/FalstinRegular-XOr2.ttf"), &gm->font))
		Platform_ExitWithErrorMessage(Str("Could not load font './assets/FalstinRegular-XOr2.ttf'."));
	if (!Render_LoadTextureFromFile(Str("./assets/base_texture.png"), &gm->base_texture))
		Platform_ExitWithErrorMessage(Str("Could not sprites from file './assets/base_texture.png'."));
	
	gm->camera_target_zoom = gm->camera_zoom = 0.25f;
}

internal void
UpdateAndRenderGame(Engine_Data* g)
{
	Game_Data* gm = g->game;
	Input_Mouse mouse;
	Input_GetMouse(&mouse);
	
	if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape) || Platform_WindowShouldClose())
		g->running = false;
	
	//~ NOTE(ljre): Update
	// Camera Movement
	{
		const float32 camera_top_speed = 10.0f / (gm->camera_zoom + 1.0f);
		vec2 dir = {
			(float32)(Input_KeyboardIsDown('D') - Input_KeyboardIsDown('A')),
			(float32)(Input_KeyboardIsDown('S') - Input_KeyboardIsDown('W')),
		};
		
		if (dir[0] != 0.0f && dir[1] != 0.0f)
			glm_vec2_scale(dir, GLM_SQRT2f / 2.0f, dir);
		
		glm_vec2_scale(dir, camera_top_speed, dir);
		glm_vec2_lerp(gm->camera_speed, dir, 0.3f, gm->camera_speed);
		glm_vec2_add(gm->camera_speed, gm->camera_pos, gm->camera_pos);
	}
	
	//- NOTE(ljre): Camera Zoom
	{
		float32 diff = (float32)mouse.scroll * 0.025f;
		
		gm->camera_target_zoom = glm_clamp(gm->camera_target_zoom + diff, 0.0f, 1.0f);
		gm->camera_zoom = glm_lerp(gm->camera_zoom, gm->camera_target_zoom, 0.3f);
	}
	
	//~ NOTE(ljre): Render
	Render_ClearBackground(0.1f, 0.1f, 0.1f, 1.0f);
	//Render_Begin2D();
	
	const int32 size = 8;
	int32 sprite_count = size * size;
	Render_Sprite2D* sprites = Arena_PushDirty(g->temp_arena, sizeof(*sprites) * sprite_count);
	
	for (int32 y = 0; y < size; ++y)
	{
		for (int32 x = 0; x < size; ++x)
		{
			Render_Sprite2D* spr = &sprites[x + y * size];
			
			glm_mat4_identity(spr->transform);
			glm_translate(spr->transform, (vec3) { (float32)x * 32.0f, (float32)y * 32.0f });
			glm_scale(spr->transform, (vec3) { 32.0f, 32.0f, 1.0f });
			
			glm_vec2_zero(&spr->texcoords[0]);
			glm_vec2_copy((vec2) { 1.0f / 128.0f * 32.0f, 1.0f / 128.0f * 32.0f }, &spr->texcoords[2]);
			
			glm_vec4_one(spr->color);
		}
	}
	
	Render_Layer2D layer = {
		.texture = &gm->base_texture,
		.sprite_count = sprite_count,
		.sprites = sprites,
	};
	
	Render_Camera camera = {
		.pos = { 0 },
		.size = { (float32)Platform_WindowWidth(), (float32)Platform_WindowHeight() },
		.zoom = gm->camera_zoom*gm->camera_zoom * 9.0f + 1.0f,
	};
	
	glm_vec2_copy(gm->camera_pos, camera.pos);
	
	mat4 view;
	Render_CalcViewMatrix2D(&camera, view);
	
	Render_DrawLayer2D(&layer, view);
	Engine_FinishFrame();
}

API void
Game_Main(Engine_Data* g)
{
	if (!g->game)
		InitGame(g);
	
	UpdateAndRenderGame(g);
	Arena_Clear(g->temp_arena);
}
