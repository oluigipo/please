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

struct Game_TroopData
{
	Game_TroopKind kind;
	
	vec2 pos;
	int32 life;
}
typedef Game_TroopData;

struct Game_Troop
{
	uint32 generation;
	
	union
	{
		Game_TroopData data;
		
		struct
		{
			Game_TroopKind padding_;
			int32 next_free;
		};
	};
}
typedef Game_Troop;

struct Game_TroopHandle
{
	union
	{
		// if == 0, null handle
		uint32 generation;
		uint32 valid; // NOTE(ljre): just here so we can do 'if (handle.valid)' to check for null
	};
	uint16 player;
	uint16 index;
}
typedef Game_TroopHandle;

struct Game_PlayerData
{
	vec4 color;
	int32 troop_count;
	int32 free_troop;
	Game_Troop troops[5];
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
			Game_TroopHandle troop;
			vec2 target_pos;
		}
		moving_troop;
		
		struct
		{
			Game_TroopHandle troop;
			Game_TroopHandle other_troop;
			
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

internal inline Game_TroopHandle
MakeTroopHandle(uint32 player_index, uint32 troop_index)
{
	Assert(player_index < ArrayLength(game->players));
	Assert(troop_index < ArrayLength(game->players[player_index].troops));
	
	return (Game_TroopHandle) {
		.generation = game->players[player_index].troops[troop_index].generation,
		.player = (uint16)player_index,
		.index = (uint16)troop_index,
	};
}

internal void
Game_PushAudio(const Asset_SoundBuffer* sound)
{
	if (game->playing_audio_count < Game_MAX_PLAYING_AUDIOS)
	{
		int32 index = game->playing_audio_count++;
		Engine_PlayingAudio* playing = &game->playing_audios[index];
		
		playing->sound = sound;
		playing->frame_index = -1;
		playing->loop = false;
		playing->volume = 0.25f;
		playing->speed = 1.0f;
	}
}

internal Game_TroopHandle
Game_FindTroopAt(const vec2 pos, Game_TroopData** out_data)
{
	Game_TroopHandle result = { 0 };
	
	int32 x = (int32)pos[0];
	int32 y = (int32)pos[1];
	
	for (uint32 i = 0; i < ArrayLength(game->players); ++i)
	{
		for (uint32 j = 0; j < game->players[i].troop_count; ++j)
		{
			if (!game->players[i].troops[j].data.kind)
				continue;
			
			int32 tx = (int32)game->players[i].troops[j].data.pos[0];
			int32 ty = (int32)game->players[i].troops[j].data.pos[1];
			
			if (x == tx && y == ty)
			{
				result.player = (uint16)i;
				result.index = (uint16)j;
				result.generation = game->players[i].troops[j].generation;
				
				if (out_data)
					*out_data = &game->players[i].troops[j].data;
				
				return result;
			}
		}
	}
	
	if (out_data)
		*out_data = NULL;
	
	return result;
}

internal inline Game_TroopData*
Game_FetchTroopData(Game_TroopHandle handle)
{
	if (!handle.valid)
		return NULL;
	
	Assert(handle.player < ArrayLength(game->players));
	Assert(handle.index < ArrayLength(game->players[handle.player].troops));
	
	Game_Troop* troop = &game->players[handle.player].troops[handle.index];
	
	if (!troop->data.kind || troop->generation != handle.generation)
		return NULL;
	
	return &troop->data;
}

internal void
Game_KillTroop(Game_TroopHandle handle)
{
	if (!handle.valid)
		return;
	
	Assert(handle.player < ArrayLength(game->players));
	Assert(handle.index < ArrayLength(game->players[handle.player].troops));
	
	Game_PlayerData* player = &game->players[handle.player];
	Game_Troop* troop = &player->troops[handle.index];
	
	if (!troop->data.kind || troop->generation != handle.generation)
		return;
	
	troop->data.kind = Game_TroopKind_Null;
	troop->next_free = player->free_troop;
	player->free_troop = handle.index;
}

internal Game_TroopHandle
Game_CreateTroop(uint32 player_index, Game_TroopData** out_data)
{
	Assert(player_index < ArrayLength(game->players));
	
	Game_TroopHandle result = { 0 };
	Game_Troop* troop = NULL;
	Game_PlayerData* player = &game->players[player_index];
	
	if (player->free_troop != -1)
	{
		troop = &player->troops[player->free_troop];
		player->free_troop = troop->next_free;
	}
	else if (player->troop_count < ArrayLength(player->troops))
	{
		troop = &player->troops[player->troop_count++];
	}
	else
		return result;
	
	++troop->generation;
	
	if (out_data)
		*out_data = &troop->data;
	
	result.generation = troop->generation;
	result.player = (uint16)player_index;
	result.index = (uint16)(troop - player->troops);
	
	return result;
}

//- NOTE(ljre): Event Functions
internal Game_Event*
Game_PushEvent(void)
{
	Assert(game->event_count < ArrayLength(game->events));
	return MemSet(&game->events[game->event_count++], 0, sizeof(Game_Event));
}

internal void
Game_NextEvent(void)
{
	Assert(game->event_count > 0);
	MemMove(&game->events[0], &game->events[1], --game->event_count * sizeof(Game_Event));
}

internal Game_Event*
Game_PushImmediateEvent(void)
{
	if (game->event_count <= 0)
		game->event_count = 1;
	return MemSet(&game->events[0], 0, sizeof(Game_Event));
}

internal void
Game_ProcessEvent(void)
{
	Assert(game->event_count > 0);
	
	Game_Event* event = &game->events[0];
	bool32 event_is_done = false;
	
	switch (event->kind)
	{
		case Game_EventKind_Null: event_is_done = true; break;
		
		case Game_EventKind_MovingTroop:
		{
			Game_TroopData* troop = Game_FetchTroopData(event->moving_troop.troop);
			
			if (!troop)
			{
				event_is_done = true;
				break;
			}
			
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
			Game_TroopData* troop = Game_FetchTroopData(event->attacking.troop);
			if (!troop)
			{
				event_is_done = true;
				break;
			}
			
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
				Game_TroopData* other_troop = Game_FetchTroopData(event->attacking.other_troop);
				if (!other_troop)
				{
					event_is_done = true;
					break;
				}
				
				troop->pos[0] = Approach(troop->pos[0], other_troop->pos[0], 1.5f);
				troop->pos[1] = Approach(troop->pos[1], other_troop->pos[1], 1.5f);
				
				if (CompareDistance(troop->pos, other_troop->pos, 8.0f) < 0)
				{
					event->attacking.back = true;
					--other_troop->life;
					
					if (other_troop->life <= 0)
						Game_KillTroop(event->attacking.other_troop);
				}
			}
		} break;
		
		default: Unreachable(); break;
	}
	
	if (event_is_done)
		Game_NextEvent();
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
	
	if (!engine->render->load_font_from_file(Str("./assets/FalstinRegular-XOr2.ttf"), &game->font))
		Platform_ExitWithErrorMessage(Str("Could not load font './assets/FalstinRegular-XOr2.ttf'."));
	if (!engine->render->load_texture_from_file(Str("./assets/base_texture.png"), &game->base_texture))
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
	
	if (PlsDiscord_Init(778719957956689922LL, &game->discord))
		PlsDiscord_UpdateActivity(&game->discord);
	
	// NOTE(ljre): Player Troops
	{
		Game_PlayerData* player = &game->players[0];
		
		player->free_troop = -1;
		player->troop_count = ArrayLength(player->troops);
		for (int32 i = 0; i < ArrayLength(player->troops); ++i)
		{
			player->troops[i] = (Game_Troop) {
				.generation = 1,
				.data = {
					.kind = Game_TroopKind_Knight,
					.pos = { i * 16.0f, i * 16.0f },
					.life = 5,
				},
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
		
		player->free_troop = -1;
		player->troop_count = ArrayLength(player->troops);
		for (int32 i = 0; i < ArrayLength(player->troops); ++i)
		{
			player->troops[i] = (Game_Troop) {
				.generation = 1,
				.data = {
					.kind = Game_TroopKind_Knight,
					.pos = { i * 16.0f, -i * 16.0f + 160.0f },
					.life = 5,
				},
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
	Engine_MouseState mouse = engine->input->mouse;
	
	if (Engine_IsPressed(engine->input->keyboard, Engine_KeyboardKey_Escape) || engine->platform->window_should_close)
		engine->running = false;
	
	//~ NOTE(ljre): Update
	Render_Camera2D camera = {
		.pos = { game->camera_pos[0], game->camera_pos[1] },
		.size = { engine->platform->window_width, engine->platform->window_height },
		.zoom = game->camera_zoom*game->camera_zoom * 9.0f + 1.0f,
	};
	
	vec2 mouse_pos;
	Engine_CalcPointInCamera2DSpace(&camera, mouse.pos, mouse_pos);
	
	if (game->event_count > 0)
	{
		//- NOTE(ljre): Events
		Game_ProcessEvent();
		
		game->camera_speed[0] = glm_lerp(game->camera_speed[0], 0.0f, 0.3f);
		game->camera_speed[1] = glm_lerp(game->camera_speed[1], 0.0f, 0.3f);
		
		game->camera_pos[0] += game->camera_speed[0];
		game->camera_pos[1] += game->camera_speed[1];
	}
	else
	{
		//- NOTE(ljre): Mouse Click
		if (Engine_IsPressed(mouse, Engine_MouseButton_Left))
		{
			if (game->selected_troop_index != -1)
			{
				// NOTE(ljre): There's already a troop selected -- do an action with it.
				Game_TroopData* troop = &game->players[game->player_turn].troops[game->selected_troop_index].data;
				Game_TroopHandle troop_handle = MakeTroopHandle(game->player_turn, game->selected_troop_index);
				vec2 target_pos;
				
				target_pos[0] = floorf(mouse_pos[0] / 16.0f) * 16.0f;
				target_pos[1] = floorf(mouse_pos[1] / 16.0f) * 16.0f;
				
				vec2 pos_diff = {
					fabsf(target_pos[0] - troop->pos[0]) / 16.0f,
					fabsf(target_pos[1] - troop->pos[1]) / 16.0f,
				};
				
				if (pos_diff[0] <= 1 && pos_diff[1] <= 1)
				{
					Game_TroopHandle other_troop_handle = Game_FindTroopAt(target_pos, NULL);
					
					if (!other_troop_handle.valid)
					{
						Game_Event* event;
						
						// NOTE(ljre): Move Troop
						event = Game_PushEvent();
						
						event->kind = Game_EventKind_MovingTroop;
						event->moving_troop.troop = troop_handle;
						
						event->moving_troop.target_pos[0] = target_pos[0];
						event->moving_troop.target_pos[1] = target_pos[1];
						
						// NOTE(ljre): Next Turn
						event = Game_PushEvent();
						
						event->kind = Game_EventKind_NextTurn;
					}
					else if (troop_handle.player != other_troop_handle.player)
					{
						Game_Event* event;
						
						// NOTE(ljre): Attack
						event = Game_PushEvent();
						
						event->kind = Game_EventKind_Attacking;
						event->attacking.troop = troop_handle;
						event->attacking.other_troop = other_troop_handle;
						event->attacking.original_troop_pos[0] = troop->pos[0];
						event->attacking.original_troop_pos[1] = troop->pos[1];
						
						// NOTE(ljre): Next Turn
						event = Game_PushEvent();
						
						event->kind = Game_EventKind_NextTurn;
					}
				}
				
				game->selected_troop_index = -1;
			}
			else
			{
				// NOTE(ljre): No troop selected -- select one.
				
				for (int32 i = 0; i < game->players[game->player_turn].troop_count; ++i)
				{
					if (game->players[game->player_turn].troops[i].data.kind &&
						CollisionPointAabb(mouse_pos, game->players[game->player_turn].troops[i].data.pos, 16.0f))
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
				Engine_IsDown(engine->input->keyboard, 'D') - Engine_IsDown(engine->input->keyboard, 'A'),
				Engine_IsDown(engine->input->keyboard, 'S') - Engine_IsDown(engine->input->keyboard, 'W'),
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
	engine->render->begin();
	engine->render->clear_background(0.1f, 0.1f, 0.1f, 1.0f);
	
	const int32 table_size = 20;
	
	int32 sprite_count = table_size*table_size
		+ ArrayLength(game->players[0].troops) * ArrayLength(game->players)
		+ 1 + (game->selected_troop_index != -1);
	
	Render_Data2DInstance* sprites = Arena_Push(engine->temp_arena, sizeof(*sprites) * sprite_count);
	Render_Data2DInstance* spr = sprites;
	
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
		
		for (int32 i = 0; i < player->troop_count; ++i)
		{
			Game_TroopData* troop = &player->troops[i].data;
			if (!troop->kind)
				continue;
			
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
		Game_TroopData* troop = &game->players[game->player_turn].troops[game->selected_troop_index].data;
		
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
	
	Render_Data2D render_data = {
		.texture = &game->base_texture,
		.camera = camera,
		.instance_count = sprite_count,
		.instances = sprites,
	};
	
	Render_Draw2D(engine, &render_data);
	
	char buff[256];
	SPrintf(buff, sizeof(buff), "%f\n%f\n", mouse_pos[0], mouse_pos[1]);
	engine->render->draw_text(&game->font, Str(buff), (vec3) { 5.0f, 5.0f }, 30.0f, GLM_VEC4_ONE, (vec3) { 0.0f });
	
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
