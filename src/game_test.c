#define Game_MAX_PLAYING_AUDIOS 32
#define Game_TABLE_SIZE 16

enum Game_Piece
{
	Game_Piece_None,
	Game_Piece_Blue,
	Game_Piece_Red,
}
typedef Game_Piece;

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
		
		Game_Piece table[Game_TABLE_SIZE][Game_TABLE_SIZE];
		
		int32 selected_piece;
		Game_Piece turn;
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
InitGame(Engine_Data* g)
{
	Game_Data* gm = g->game = Arena_Push(g->persistent_arena, sizeof(*g->game));
	gm->master_volume = 0.25f;
	
	if (!Render_LoadFontFromFile(Str("./assets/FalstinRegular-XOr2.ttf"), &gm->font))
		Platform_ExitWithErrorMessage(Str("Could not load font './assets/FalstinRegular-XOr2.ttf'."));
	if (!Render_LoadTextureFromFile(Str("./assets/base_texture.png"), &gm->base_texture))
		Platform_ExitWithErrorMessage(Str("Could not sprites from file './assets/base_texture.png'."));
	
	gm->camera_target_zoom = gm->camera_zoom = 0.25f;
	gm->selected_piece = -1;
	gm->turn = Game_Piece_Red;
	
	for (int32 y = 0; y < Game_TABLE_SIZE; ++y)
	{
		for (int32 x = 0; x < Game_TABLE_SIZE; ++x)
		{
			if (y < 2 && x % 2 != y % 2)
				gm->table[y][x] = Game_Piece_Red;
			else if (y >= Game_TABLE_SIZE - 2 && x % 2 != y % 2)
				gm->table[y][x] = Game_Piece_Blue;
			else
				gm->table[y][x] = Game_Piece_None;
		}
	}
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
	
	//- NOTE(ljre): Camera Matrix
	Render_Camera camera = {
		.pos = { 0 },
		.size = { (float32)Platform_WindowWidth(), (float32)Platform_WindowHeight() },
		.zoom = gm->camera_zoom*gm->camera_zoom * 9.0f + 1.0f,
	};
	
	glm_vec2_copy(gm->camera_pos, camera.pos);
	
	mat4 view;
	Render_CalcViewMatrix2D(&camera, view);
	
	//- NOTE(ljre): Mouse Click
	vec2 mouse_pos;
	Render_CalcPointInCamera2DSpace(&camera, mouse.pos, mouse_pos);
	
	if (Input_IsPressed(mouse, Input_MouseButton_Left))
	{
		vec2 click_pos;
		glm_vec2_copy(mouse_pos, click_pos);
		
		glm_vec2_divs(click_pos, 16.0f, click_pos);
		
		int32 xx =  (int32)click_pos[0];
		int32 yy = -(int32)click_pos[1];
		
		if (xx >= 0 && xx < Game_TABLE_SIZE && yy >= 0 && yy < Game_TABLE_SIZE)
		{
			if (gm->selected_piece != -1)
			{
				int32 to_move = gm->selected_piece;
				gm->selected_piece = -1;
				
				int32 move_x = to_move % Game_TABLE_SIZE;
				int32 move_y = to_move / Game_TABLE_SIZE;
				
				if (move_x >= 0 && move_x < Game_TABLE_SIZE && move_y >= 0 && move_y < Game_TABLE_SIZE &&
					gm->table[move_y][move_x] == gm->turn &&
					gm->table[yy][xx] == Game_Piece_None &&
					(gm->turn == Game_Piece_Blue ? move_y > yy : move_y < yy) &&
					move_x != xx)
				{
					int32 diff_x = move_x - xx;
					int32 diff_y = move_y - yy;
					
					if ((diff_y == 2 || diff_y == -2) && (diff_x == 2 || diff_x == -2))
					{
						int32 eating_x = xx + diff_x/2;
						int32 eating_y = yy + diff_y/2;
						
						if (gm->table[eating_y][eating_x] != gm->turn)
						{
							gm->table[eating_y][eating_x] = Game_Piece_None;
							gm->table[yy][xx] = gm->turn;
							gm->table[move_y][move_x] = Game_Piece_None;
							
							gm->turn = !(gm->turn-1) + 1;
						}
					}
					else if ((diff_x == 1 || diff_x == -1) && (diff_y == 1 || diff_y == -1) &&
							 gm->table[yy][xx] == Game_Piece_None)
					{
						gm->table[yy][xx] = gm->turn;
						gm->table[move_y][move_x] = Game_Piece_None;
						gm->turn = !(gm->turn-1) + 1;
					}
				}
			}
			else
			{
				gm->selected_piece = xx + yy * Game_TABLE_SIZE;
			}
		}
	}
	
	//~ NOTE(ljre): Render
	Render_Begin();
	Render_ClearBackground(0.1f, 0.1f, 0.1f, 1.0f);
	
	int32 sprite_count = Game_TABLE_SIZE * Game_TABLE_SIZE * 2;
	Render_Sprite2D* sprites = Arena_PushDirty(g->temp_arena, sizeof(*sprites) * sprite_count);
	Render_Sprite2D* spr = sprites;
	
	for (int32 y = 0; y < Game_TABLE_SIZE; ++y)
	{
		for (int32 x = 0; x < Game_TABLE_SIZE; ++x)
		{
			vec4 black = { 24.0f / 255.0f, 20.0f / 255.0f, 37.0f / 255.0f, 1.0f };
			vec4 white = GLM_VEC4_ONE_INIT;
			
			glm_mat4_identity(spr->transform);
			glm_translate(spr->transform, (vec3) { (float32)x * 16.0f, (float32)y * 16.0f });
			glm_scale(spr->transform, (vec3) { 16.0f, 16.0f, 1.0f });
			
			glm_vec2_zero(&spr->texcoords[0]);
			glm_vec2_copy((vec2) { 16.0f / 128.0f, 16.0f / 128.0f }, &spr->texcoords[2]);
			
			if ((x + y) % 2 == 0)
				glm_vec4_copy(black, spr->color);
			else
				glm_vec4_copy(white, spr->color);
			
			if (gm->table[y][x])
			{
				glm_mat4_copy(spr->transform, spr[1].transform);
				++spr;
				
				glm_vec4_copy((vec4) { 16.0f / 128.0f, 0.0f, 16.0f / 128.0f, 16.0f / 128.0f }, spr->texcoords);
				
				switch (gm->table[y][x])
				{
					case Game_Piece_Red: glm_vec4_copy((vec4) { 0.8f, 0.05f, 0.1f, 1.0f }, spr->color); break;
					case Game_Piece_Blue: glm_vec4_copy((vec4) { 0.1f, 0.05f, 0.8f, 1.0f }, spr->color); break;
					default:;
				}
			}
			else
			{
				sprite_count--;
			}
			
			++spr;
		}
	}
	
	Render_Layer2D layer = {
		.texture = &gm->base_texture,
		.sprite_count = sprite_count,
		.sprites = sprites,
	};
	
	Render_DrawLayer2D(&layer, view);
	
	char buff[256];
	snprintf(buff, sizeof buff, "%f\n%f\n%f\n%f", mouse_pos[0], mouse_pos[1]);
	Render_DrawText(&gm->font, Str(buff), (vec3) { 5.0f, 5.0f }, 30.0f, GLM_VEC4_ONE, (vec3) { 0.0f });
	
	Engine_FinishFrame();
}

API void
Game_Main(Engine_Data* g)
{
	if (!g->game)
		InitGame(g);
	
	Arena_Clear(g->temp_arena);
	UpdateAndRenderGame(g);
}
