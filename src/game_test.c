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
	int32 life;
}
typedef Game_Troop;

struct Game_PlayerData
{
	vec4 color;
	Game_Troop troops[5];
	int32 troop_count;
}
typedef Game_PlayerData;

//- NOTE(ljre): Event Types
enum Game_EventKind
{
	Game_EventKind_Null = 0,
	
	Game_EventKind_MovingTroop,
	Game_EventKind_NextTurn,
	Game_EventKind_Attacking,
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
		
		struct
		{
			Game_Troop* troop;
			Game_Troop* other_troop;
			
			vec2 original_troop_pos;
			bool32 back;
		}
		attacking;
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
		
		Game_Event events[32];
		int32 event_count;
		
		int32 player_turn;
		int32 selected_troop_index;
		
		Game_PlayerData players[2];
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

internal float32
Approach(float32 current, float32 target, float32 step)
{
	if (current < target)
		return glm_min(current + step, target);
	
	return glm_max(current - step, target);
}

internal int32
CompareDistance(const vec2 a, const vec2 b, float32 cmp)
{
	vec2 diff;
	
	diff[0] = a[0] - b[0];
	diff[1] = a[1] - b[1];
	
	diff[0] *= diff[0];
	diff[1] *= diff[1];
	
	cmp *= cmp;
	
	return (int32)glm_signf(diff[0] + diff[1] - cmp);
}

internal Game_Troop*
FindTroopAt(const vec2 pos)
{
	int32 x = (int32)pos[0];
	int32 y = (int32)pos[1];
	
	for (int32 i = 0; i < ArrayLength(game->players); ++i)
	{
		for (int32 j = 0; j < ArrayLength(game->players[i].troops); ++j)
		{
			int32 tx = (int32)game->players[i].troops[j].pos[0];
			int32 ty = (int32)game->players[i].troops[j].pos[1];
			
			if (x == tx && y == ty)
				return &game->players[i].troops[j];
		}
	}
	
	return NULL;
}

internal Game_PlayerData*
FindPlayerFromTroop(Game_Troop* troop)
{
	for (int32 i = 0; i < ArrayLength(game->players); ++i)
	{
		if (troop >= game->players[i].troops &&
			troop < game->players[i].troops + ArrayLength(game->players[i].troops))
		{
			return &game->players[i];
		}
	}
	
	return NULL;
}

internal inline bool32
AreTroopsFromSamePlayer(Game_Troop* a, Game_Troop* b)
{
	return FindPlayerFromTroop(a) == FindPlayerFromTroop(b);
}

//- NOTE(ljre): Event Functions
internal Game_Event*
PushEvent(void)
{
	Assert(game->event_count < ArrayLength(game->events));
	return MemSet(&game->events[game->event_count++], 0, sizeof(Game_Event));
}

internal void
NextEvent(void)
{
	Assert(game->event_count > 0);
	MemMove(&game->events[0], &game->events[1], --game->event_count * sizeof(Game_Event));
}

internal Game_Event*
PushImmediateEvent(void)
{
	if (game->event_count <= 0)
		game->event_count = 1;
	return MemSet(&game->events[0], 0, sizeof(Game_Event));
}

internal void
ProcessEvent(void)
{
	Assert(game->event_count > 0);
	
	Game_Event* event = &game->events[0];
	bool32 event_is_done = false;
	
	switch (event->kind)
	{
		case Game_EventKind_Null: event_is_done = true; break;
		
		case Game_EventKind_MovingTroop:
		{
			Game_Troop* troop = event->moving_troop.troop;
			
			troop->pos[0] = glm_lerp(troop->pos[0], event->moving_troop.target_pos[0], 0.2f);
			troop->pos[1] = glm_lerp(troop->pos[1], event->moving_troop.target_pos[1], 0.2f);
			
			if (CompareDistance(troop->pos, event->moving_troop.target_pos, 0.1f) < 0)
			{
				troop->pos[0] = event->moving_troop.target_pos[0];
				troop->pos[1] = event->moving_troop.target_pos[1];
				
				event_is_done = true;
			}
		} break;
		
		case Game_EventKind_NextTurn:
		{
			game->player_turn = (game->player_turn + 1) % ArrayLength(game->players);
			event_is_done = true;
		} break;
		
		case Game_EventKind_Attacking:
		{
			Game_Troop* troop = event->attacking.troop;
			Game_Troop* other_troop = event->attacking.other_troop;
			
			if (event->attacking.back)
			{
				troop->pos[0] = glm_lerp(troop->pos[0], event->attacking.original_troop_pos[0], 0.3f);
				troop->pos[1] = glm_lerp(troop->pos[1], event->attacking.original_troop_pos[1], 0.3f);
				
				if (CompareDistance(troop->pos, event->attacking.original_troop_pos, 0.5f) < 0)
				{
					troop->pos[0] = event->attacking.original_troop_pos[0];
					troop->pos[1] = event->attacking.original_troop_pos[1];
					
					event_is_done = true;
				}
			}
			else
			{
				troop->pos[0] = Approach(troop->pos[0], other_troop->pos[0], 1.5f);
				troop->pos[1] = Approach(troop->pos[1], other_troop->pos[1], 1.5f);
				
				if (CompareDistance(troop->pos, other_troop->pos, 8.0f) < 0)
				{
					event->attacking.back = true;
					--other_troop->life;
				}
			}
		} break;
		
		default: Unreachable(); break;
	}
	
	if (event_is_done)
		NextEvent();
}

//- NOTE(ljre): Init Game Function
internal void
Game_Init(Game_Data* game)
{
	*game = (Game_Data) {
		.master_volume = 0.25f,
		.camera_target_zoom = 0.25f,
		.camera_zoom = 0.25f,
		.selected_troop_index = -1,
	};
	
	if (!Render_LoadFontFromFile(Str("./assets/FalstinRegular-XOr2.ttf"), &game->font))
		Platform_ExitWithErrorMessage(Str("Could not load font './assets/FalstinRegular-XOr2.ttf'."));
	if (!Render_LoadTextureFromFile(Str("./assets/base_texture.png"), &game->base_texture))
		Platform_ExitWithErrorMessage(Str("Could not load sprites from file './assets/base_texture.png'."));
	
	game->discord.activity = (struct DiscordActivity) {
		.assets = {
			.large_image = "eredin",
			.large_text = "cat",
			.small_image = "preto",
			.small_text = "white",
		},
		
		.type = DiscordActivityType_Playing,
		.name = "Name",
		.state = "State",
		.details = "Details",
	};
	
	PlsDiscord_Init(778719957956689922LL, &game->discord);
	PlsDiscord_UpdateActivity(&game->discord);
	
	// NOTE(ljre): Player Troops
	{
		Game_PlayerData* player = &game->players[0];
		
		player->troop_count = ArrayLength(player->troops);
		for (int32 i = 0; i < ArrayLength(player->troops); ++i)
		{
			player->troops[i] = (Game_Troop) {
				.kind = Game_TroopKind_Knight,
				.pos = { i * 16.0f, i * 16.0f },
				.life = 5,
			};
		}
		
		player->color[0] = 1.0f;
		player->color[1] = 1.0f;
		player->color[2] = 1.0f;
		player->color[3] = 1.0f;
	}
	
	// NOTE(ljre): Enemy Troops
	{
		Game_PlayerData* player = &game->players[1];
		
		player->troop_count = ArrayLength(player->troops);
		for (int32 i = 0; i < ArrayLength(player->troops); ++i)
		{
			player->troops[i] = (Game_Troop) {
				.kind = Game_TroopKind_Knight,
				.pos = { i * 16.0f, -i * 16.0f + 160.0f },
				.life = 5,
			};
		}
		
		player->color[0] = 1.0f;
		player->color[1] = 0.5f;
		player->color[2] = 0.5f;
		player->color[3] = 1.0f;
	}
}

//- NOTE(ljre): Update Game Function
internal void
ProcessDiscordEvents(void)
{
	if (0 && game->discord.connected)
	{
		if (!game->discord.lobby.id && !game->discord.connecting_to_lobby)
			PlsDiscord_CreateLobby(&game->discord);
	}
	
	PlsDiscord_Update(&game->discord);
}

internal void
Game_UpdateAndRender(void)
{
	Input_Mouse mouse;
	Input_GetMouse(&mouse);
	
	if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape) || Platform_WindowShouldClose())
		engine->running = false;
	
	//~ NOTE(ljre): Update
	Render_Camera camera = {
		.pos = { game->camera_pos[0], game->camera_pos[1] },
		.size = { Platform_WindowWidth(), Platform_WindowHeight() },
		.zoom = game->camera_zoom*game->camera_zoom * 9.0f + 1.0f,
	};
	
	vec2 mouse_pos;
	Render_CalcPointInCamera2DSpace(&camera, mouse.pos, mouse_pos);
	
	if (game->event_count > 0)
	{
		//- NOTE(ljre): Events
		ProcessEvent();
		
		game->camera_speed[0] = glm_lerp(game->camera_speed[0], 0.0f, 0.3f);
		game->camera_speed[1] = glm_lerp(game->camera_speed[1], 0.0f, 0.3f);
		
		game->camera_pos[0] += game->camera_speed[0];
		game->camera_pos[1] += game->camera_speed[1];
	}
	else
	{
		//- NOTE(ljre): Mouse Click
		if (Input_IsPressed(mouse, Input_MouseButton_Left))
		{
			if (game->selected_troop_index != -1)
			{
				// NOTE(ljre): There's already a troop selected -- do an action with it.
				Game_Troop* troop = &game->players[game->player_turn].troops[game->selected_troop_index];
				vec2 target_pos;
				
				target_pos[0] = floorf(mouse_pos[0] / 16.0f) * 16.0f;
				target_pos[1] = floorf(mouse_pos[1] / 16.0f) * 16.0f;
				
				vec2 pos_diff = {
					fabsf(target_pos[0] - troop->pos[0]) / 16.0f,
					fabsf(target_pos[1] - troop->pos[1]) / 16.0f,
				};
				
				if (pos_diff[0] <= 1 && pos_diff[1] <= 1)
				{
					Game_Troop* other_troop = FindTroopAt(target_pos);
					
					if (!other_troop)
					{
						Game_Event* event;
						
						// NOTE(ljre): Move Troop
						event = PushEvent();
						
						event->kind = Game_EventKind_MovingTroop;
						event->moving_troop.troop = troop;
						
						event->moving_troop.target_pos[0] = target_pos[0];
						event->moving_troop.target_pos[1] = target_pos[1];
						
						// NOTE(ljre): Next Turn
						event = PushEvent();
						
						event->kind = Game_EventKind_NextTurn;
					}
					else if (!AreTroopsFromSamePlayer(troop, other_troop))
					{
						Game_Event* event;
						
						// NOTE(ljre): Attack
						event = PushEvent();
						
						event->kind = Game_EventKind_Attacking;
						event->attacking.troop = troop;
						event->attacking.other_troop = other_troop;
						event->attacking.original_troop_pos[0] = troop->pos[0];
						event->attacking.original_troop_pos[1] = troop->pos[1];
						
						// NOTE(ljre): Next Turn
						event = PushEvent();
						
						event->kind = Game_EventKind_NextTurn;
					}
				}
				
				game->selected_troop_index = -1;
			}
			else
			{
				// NOTE(ljre): No troop selected -- select one.
				
				for (int32 i = 0; i < ArrayLength(game->players[game->player_turn].troops); ++i)
				{
					if (CollisionPointAabb(mouse_pos, game->players[game->player_turn].troops[i].pos, 16.0f))
					{
						game->selected_troop_index = i;
						break;
					}
				}
			}
		}
		
		//- NOTE(ljre): Camera Movement
		{
			const float32 camera_top_speed = 10.0f / (game->camera_zoom + 1.0f);
			vec2 dir = {
				Input_KeyboardIsDown('D') - Input_KeyboardIsDown('A'),
				Input_KeyboardIsDown('S') - Input_KeyboardIsDown('W'),
			};
			
			if (dir[0] != 0.0f && dir[1] != 0.0f)
			{
				dir[0] *= GLM_SQRT2f / 2.0f;
				dir[1] *= GLM_SQRT2f / 2.0f;
			}
			
			dir[0] *= camera_top_speed;
			dir[1] *= camera_top_speed;
			
			game->camera_speed[0] = glm_lerp(game->camera_speed[0], dir[0], 0.3f);
			game->camera_speed[1] = glm_lerp(game->camera_speed[1], dir[1], 0.3f);
			
			game->camera_pos[0] += game->camera_speed[0];
			game->camera_pos[1] += game->camera_speed[1];
		}
		
		//- NOTE(ljre): Camera Zoom
		{
			float32 diff = mouse.scroll * 0.025f;
			
			game->camera_target_zoom = glm_clamp(game->camera_target_zoom + diff, 0.0f, 1.0f);
			game->camera_zoom = glm_lerp(game->camera_zoom, game->camera_target_zoom, 0.3f);
		}
	}
	
	//~ NOTE(ljre): Render
	Render_Begin();
	Render_ClearBackground(0.1f, 0.1f, 0.1f, 1.0f);
	
	const int32 table_size = 20;
	
	int32 sprite_count = table_size*table_size
		+ ArrayLength(game->players[0].troops) * ArrayLength(game->players)
		+ 1 + (game->selected_troop_index != -1);
	
	Render_Sprite2D* sprites = Arena_PushDirty(engine->temp_arena, sizeof(*sprites) * sprite_count);
	Render_Sprite2D* spr = sprites;
	
	for (int32 y = 0; y < table_size; ++y)
	{
		for (int32 x = 0; x < table_size; ++x)
		{
			glm_mat4_identity(spr->transform);
			glm_translate(spr->transform, (vec3) { x * 16.0f, y * 16.0f });
			glm_scale(spr->transform, (vec3) { 16.0f, 16.0f, 1.0f });
			
			spr->texcoords[0] = 0.0f;
			spr->texcoords[1] = 16.0f / 128.0f;
			spr->texcoords[2] = 16.0f / 128.0f;
			spr->texcoords[3] = 16.0f / 128.0f;
			
			spr->color[0] = 1.0f;
			spr->color[1] = 1.0f;
			spr->color[2] = 1.0f;
			spr->color[3] = 1.0f;
			
			++spr;
		}
	}
	
	for (int32 i = 0; i < ArrayLength(game->players); ++i)
	{
		Game_PlayerData* player = &game->players[i];
		
		for (int32 i = 0; i < ArrayLength(player->troops); ++i)
		{
			Game_Troop* troop = &player->troops[i];
			
			glm_mat4_identity(spr->transform);
			glm_translate(spr->transform, (vec3) { troop->pos[0], troop->pos[1], 0.0f });
			glm_scale(spr->transform, (vec3) { 16.0f, 16.0f, 1.0f });
			
			spr->texcoords[0] = 16.0f / 128.0f;
			spr->texcoords[1] = 0.0f;
			spr->texcoords[2] = 16.0f / 128.0f;
			spr->texcoords[3] = 16.0f / 128.0f;
			
			spr->color[0] = player->color[0];
			spr->color[1] = player->color[1];
			spr->color[2] = player->color[2];
			spr->color[3] = player->color[3];
			
			++spr;
		}
	}
	
	// NOTE(ljre): Mouse Position
	{
		float32 scl = 1.1f + sinf((float32)engine->last_frame_time) * 0.05f;
		
		vec3 pos = {
			floorf(mouse_pos[0] / 16.0f) * 16.0f - (scl - 1.0f) * 8.0f,
			floorf(mouse_pos[1] / 16.0f) * 16.0f - (scl - 1.0f) * 8.0f,
			1.0f
		};
		
		glm_mat4_identity(spr->transform);
		glm_translate(spr->transform, pos);
		glm_scale(spr->transform, (vec3) { 16.0f * scl, 16.0f * scl, 1.0f });
		
		spr->texcoords[0] = 16.0f / 128.0f;
		spr->texcoords[1] = 16.0f / 128.0f;
		spr->texcoords[2] = 16.0f / 128.0f;
		spr->texcoords[3] = 16.0f / 128.0f;
		
		spr->color[0] = 1.0f;
		spr->color[1] = 1.0f;
		spr->color[2] = 1.0f;
		spr->color[3] = 1.0f;
		
		++spr;
	}
	
	if (game->selected_troop_index != -1)
	{
		Game_Troop* troop = &game->players[game->player_turn].troops[game->selected_troop_index];
		
		vec3 pos = {
			troop->pos[0],
			troop->pos[1],
			1.0f
		};
		
		glm_mat4_identity(spr->transform);
		glm_translate(spr->transform, pos);
		glm_scale(spr->transform, (vec3) { 16.0f, 16.0f, 1.0f });
		
		spr->texcoords[0] = 16.0f / 128.0f;
		spr->texcoords[1] = 16.0f / 128.0f;
		spr->texcoords[2] = 16.0f / 128.0f;
		spr->texcoords[3] = 16.0f / 128.0f;
		
		spr->color[0] = 1.0f;
		spr->color[1] = 1.0f;
		spr->color[2] = 1.0f;
		spr->color[3] = 1.0f;
		
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
	
	ProcessDiscordEvents();
	Engine_FinishFrame();
}

//~ NOTE(ljre): Public API
API void
Game_Main(Engine_Data* g)
{
	engine = g;
	
	if (!g->game)
	{
		g->game = Arena_Push(engine->persistent_arena, sizeof(*g->game));
		Game_Init(g->game);
	}
	
	game = g->game;
	
	Arena_Clear(engine->temp_arena);
	Game_UpdateAndRender();
	
	if (!g->running)
		PlsDiscord_Deinit(&g->game->discord);
}
