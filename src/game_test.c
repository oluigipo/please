#include "system_discord.c"

#define Game_MAX_PLAYING_AUDIOS 32

// NOTE(ljre): These globals are set every frame by 'Game_Main'!
//             They exist just for less typing...
internal Engine_Data* engine;
internal Game_Data* game;

//~ NOTE(ljre): Types
enum Game_TroopKind
{
	Game_TroopKind_Null = 0,
	
	Game_TroopKind_Knight,
	Game_TroopKind_Archer,
}
typedef Game_TroopKind;

struct Game_Troop
{
	Game_TroopKind kind;
	
	vec2 pos;
}
typedef Game_Troop;

//- NOTE(ljre): Event Types
enum Game_EventKind
{
	Game_EventKind_Null = 0,
	
	Game_EventKind_MovingTroop,
}
typedef Game_EventKind;

struct Game_Event
{
	Game_EventKind kind;
	
	union
	{
		struct
		{
			Game_Troop* troop;
			vec2 target_pos;
		}
		moving_troop;
	};
}
typedef Game_Event;

//- NOTE(ljre): Main Game Data
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
	
	PlsDiscord_Client discord;
	
	// NOTE(ljre): Gameplay Data
	struct
	{
		vec2 camera_pos;
		vec2 camera_speed;
		float32 camera_zoom;
		float32 camera_target_zoom;
		
		Game_Troop troops[5];
		
		Game_Event events[32];
		int32 event_count;
		
		Game_Troop* selected_troop;
	};
};

//~ NOTE(ljre): Functions
internal void
PushAudio(const Asset_SoundBuffer* sound)
{
	if (game->playing_audio_count < Game_MAX_PLAYING_AUDIOS)
	{
		int32 index = game->playing_audio_count++;
		Engine_PlayingAudio* playing = &game->playing_audios[index];
		
		playing->sound = sound;
		playing->frame_index = -1;
		playing->loop = false;
		playing->volume = 1.0f;
		playing->speed = 1.0f;
	}
}

internal bool32
CollisionPointAabb(const vec2 point, const vec2 sqr, float32 size)
{
	return (point[0] > sqr[0] && sqr[0] + size > point[0] &&
			point[1] > sqr[1] && sqr[1] + size > point[1]);
}

//- NOTE(ljre): Event Functions
internal Game_Event*
PushEvent(void)
{
	Assert(game->event_count < ArrayLength(game->events));
	return memset(&game->events[game->event_count++], 0, sizeof(Game_Event));
}

internal void
NextEvent(void)
{
	Assert(game->event_count > 0);
	memmove(game->events, game->events + 1, --game->event_count);
}

internal Game_Event*
PushImmediateEvent(void)
{
	if (game->event_count <= 0)
		game->event_count = 1;
	return memset(&game->events[0], 0, sizeof(Game_Event));
}

internal void
ProcessEvent(void)
{
	Game_Event* event = &game->events[0];
	bool32 event_is_done = false;
	
	switch (event->kind)
	{
		case Game_EventKind_Null: break;
		
		case Game_EventKind_MovingTroop:
		{
			Game_Troop* troop = event->moving_troop.troop;
			
			glm_vec2_lerp(troop->pos, event->moving_troop.target_pos, 0.2f, troop->pos);
			
			vec2 diff;
			glm_vec2_sub(event->moving_troop.target_pos, troop->pos, diff);
			glm_vec2_mul(diff, diff, diff);
			
			if (diff[0] + diff[1] < 0.1f)
			{
				glm_vec2_copy(event->moving_troop.target_pos, troop->pos);
				event_is_done = true;
			}
		} break;
	}
	
	if (event_is_done)
		NextEvent();
}

internal void
ProcessDiscordEvents(void)
{
	if (game->discord.connected)
	{
		if (!game->discord.lobby.id && !game->discord.connecting_to_lobby)
		{
			PlsDiscord_CreateLobby(&game->discord);
		}
	}
	
	PlsDiscord_Update(&game->discord);
}

//- NOTE(ljre): Init Game Function
internal Game_Data*
InitGame(void)
{
	Game_Data* gm = Arena_Push(engine->persistent_arena, sizeof(*engine->game));
	gm->master_volume = 0.25f;
	
	if (!Render_LoadFontFromFile(Str("./assets/FalstinRegular-XOr2.ttf"), &gm->font))
		Platform_ExitWithErrorMessage(Str("Could not load font './assets/FalstinRegular-XOr2.ttf'."));
	if (!Render_LoadTextureFromFile(Str("./assets/base_texture.png"), &gm->base_texture))
		Platform_ExitWithErrorMessage(Str("Could not sprites from file './assets/base_texture.png'."));
	
	PlsDiscord_Init(778719957956689922LL, &gm->discord);
	PlsDiscord_UpdateActivityAssets(&gm->discord, Str("eredin"), Str("cat"), Str("preto"), Str("white"));
	PlsDiscord_UpdateActivity(&gm->discord, DiscordActivityType_Playing, Str("Name"), Str("State"), Str("Details"));
	
	gm->camera_target_zoom = gm->camera_zoom = 0.25f;
	
	for (int32 i = 0; i < ArrayLength(gm->troops); ++i)
		gm->troops[i] = (Game_Troop) { Game_TroopKind_Knight, .pos = {(float32)i * 16.0f, (float32)i * 16.0f}, };
	
	return gm;
}

//- NOTE(ljre): Update Game Function
internal void
UpdateAndRenderGame(void)
{
	ProcessDiscordEvents();
	
	Input_Mouse mouse;
	Input_GetMouse(&mouse);
	
	if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape) || Platform_WindowShouldClose())
		engine->running = false;
	
	//~ NOTE(ljre): Update
	Render_Camera camera = {
		.pos = { 0 },
		.size = { (float32)Platform_WindowWidth(), (float32)Platform_WindowHeight() },
		.zoom = game->camera_zoom*game->camera_zoom * 9.0f + 1.0f,
	};
	
	glm_vec2_copy(game->camera_pos, camera.pos);
	
	vec2 mouse_pos;
	Render_CalcPointInCamera2DSpace(&camera, mouse.pos, mouse_pos);
	
	if (game->event_count > 0)
	{
		//- NOTE(ljre): Events
		ProcessEvent();
		
		glm_vec2_lerp(game->camera_speed, GLM_VEC2_ZERO, 0.3f, game->camera_speed);
		glm_vec2_add(game->camera_speed, game->camera_pos, game->camera_pos);
	}
	else
	{
		//- NOTE(ljre): Mouse Click
		if (Input_IsPressed(mouse, Input_MouseButton_Left))
		{
			if (game->selected_troop)
			{
				vec2 target_pos;
				
				target_pos[0] = floorf(mouse_pos[0] / 16.0f) * 16.0f;
				target_pos[1] = floorf(mouse_pos[1] / 16.0f) * 16.0f;
				
				Game_Event* event = PushEvent();
				
				event->kind = Game_EventKind_MovingTroop;
				event->moving_troop.troop = game->selected_troop;
				glm_vec2_copy(target_pos, event->moving_troop.target_pos);
				
				game->selected_troop = NULL;
			}
			else
			{
				for (int32 i = 0; i < ArrayLength(game->troops); ++i)
				{
					if (CollisionPointAabb(mouse_pos, game->troops[i].pos, 16.0f))
					{
						game->selected_troop = &game->troops[i];
						break;
					}
				}
			}
		}
		
		//- NOTE(ljre): Camera Movement
		{
			const float32 camera_top_speed = 10.0f / (game->camera_zoom + 1.0f);
			vec2 dir = {
				(float32)(Input_KeyboardIsDown('D') - Input_KeyboardIsDown('A')),
				(float32)(Input_KeyboardIsDown('S') - Input_KeyboardIsDown('W')),
			};
			
			if (dir[0] != 0.0f && dir[1] != 0.0f)
				glm_vec2_scale(dir, GLM_SQRT2f / 2.0f, dir);
			
			glm_vec2_scale(dir, camera_top_speed, dir);
			glm_vec2_lerp(game->camera_speed, dir, 0.3f, game->camera_speed);
			glm_vec2_add(game->camera_speed, game->camera_pos, game->camera_pos);
		}
		
		//- NOTE(ljre): Camera Zoom
		{
			float32 diff = (float32)mouse.scroll * 0.025f;
			
			game->camera_target_zoom = glm_clamp(game->camera_target_zoom + diff, 0.0f, 1.0f);
			game->camera_zoom = glm_lerp(game->camera_zoom, game->camera_target_zoom, 0.3f);
		}
	}
	
	//~ NOTE(ljre): Render
	Render_Begin();
	Render_ClearBackground(0.1f, 0.1f, 0.1f, 1.0f);
	
	const int32 table_size = 20;
	
	int32 sprite_count = table_size*table_size + ArrayLength(game->troops);
	Render_Sprite2D* sprites = Arena_PushDirty(engine->temp_arena, sizeof(*sprites) * sprite_count);
	Render_Sprite2D* spr = sprites;
	
	for (int32 y = 0; y < table_size; ++y)
	{
		for (int32 x = 0; x < table_size; ++x)
		{
			glm_mat4_identity(spr->transform);
			glm_translate(spr->transform, (vec3) { (float32)x * 16.0f, (float32)y * 16.0f });
			glm_scale(spr->transform, (vec3) { 16.0f, 16.0f, 1.0f });
			
			glm_vec2_copy((vec2) { 0.0f, 16.0f / 128.0f }, &spr->texcoords[0]);
			glm_vec2_copy((vec2) { 16.0f / 128.0f, 16.0f / 128.0f }, &spr->texcoords[2]);
			
			glm_vec4_copy(GLM_VEC4_ONE, spr->color);
			
			++spr;
		}
	}
	
	for (int32 i = 0; i < ArrayLength(game->troops); ++i)
	{
		Game_Troop* troop = &game->troops[i];
		
		glm_mat4_identity(spr->transform);
		glm_translate(spr->transform, (vec3) { troop->pos[0], troop->pos[1], 0.0f });
		glm_scale(spr->transform, (vec3) { 16.0f, 16.0f, 1.0f });
		
		glm_vec4_copy((vec4) {
						  16.0f / 128.0f, 0.0f,
						  16.0f / 128.0f, 16.0f / 128.0f,
					  }, spr->texcoords);
		
		glm_vec4_copy(GLM_VEC4_ONE, spr->color);
		
		++spr;
	}
	
	Render_Layer2D layer = {
		.texture = &game->base_texture,
		.sprite_count = sprite_count,
		.sprites = sprites,
	};
	
	mat4 view;
	Render_CalcViewMatrix2D(&camera, view);
	Render_DrawLayer2D(&layer, view);
	
	char buff[256];
	snprintf(buff, sizeof buff, "%f\n%f\n", mouse_pos[0], mouse_pos[1]);
	Render_DrawText(&game->font, Str(buff), (vec3) { 5.0f, 5.0f }, 30.0f, GLM_VEC4_ONE, (vec3) { 0.0f });
	
	Engine_FinishFrame();
}

//~ NOTE(ljre): Public API
API void
Game_Main(Engine_Data* g)
{
	engine = g;
	if (!g->game)
		g->game = InitGame();
	game = g->game;
	
	Arena_Clear(engine->temp_arena);
	UpdateAndRenderGame();
	
	if (!g->running)
		PlsDiscord_Deinit(&g->game->discord);
}
