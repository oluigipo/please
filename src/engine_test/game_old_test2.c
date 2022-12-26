#include "system_discord.c"

static Engine_Data* engine;
static Game_Data* game;

static const float32 global_camera_zoom = 4.0f;

struct Game_Player
{
	vec2 pos;
	vec2 speed;
	
	int32 life;
}
typedef Game_Player;

enum Game_BulletKind
{
	Game_BulletKind_Null = 0,
	
	Game_BulletKind_Normal,
}
typedef Game_BulletKind;

struct Game_Bullet
{
	Game_BulletKind kind;
	
	vec2 pos;
	vec2 speed;
	float32 angle;
}
typedef Game_Bullet;

struct Game_Data
{
	Asset_Font font;
	Asset_Texture base_texture;
	
	Game_Player player;
	Game_Bullet friendly_bullets[100];
	int32 friendly_bullet_count;
	
	PlsDiscord_Client discord;
};

static void
Game_PushFriendlyBullet(const Game_Bullet* bullet)
{
	Assert(game->friendly_bullet_count < ArrayLength(game->friendly_bullets));
	
	game->friendly_bullets[game->friendly_bullet_count++] = *bullet;
}

static bool
Game_Aabb(const vec4 a, const vec4 b)
{
	return ((a[0] + a[2] >= b[0] && b[0] + b[2] >= a[0]) &&
		(a[1] + a[3] >= b[1] && b[1] + b[3] >= a[1]));
}

static void
Game_CameraBoundingBox(vec4 out_bbox)
{
	float32 width = engine->platform->window_width;
	float32 height = engine->platform->window_height;
	float32 inv_zoom = 1.0f / global_camera_zoom;
	
	out_bbox[0] = -width  * inv_zoom * 0.5f;
	out_bbox[1] = -height * inv_zoom * 0.5f;
	out_bbox[2] =  width  * inv_zoom;
	out_bbox[3] =  height * inv_zoom;
}

//~ Main
static void
Game_Init(void)
{
	Trace();
	
	Mem_Set(game, 0, sizeof(*game));
	
	if (!engine->render->load_font_from_file(Str("./assets/FalstinRegular-XOr2.ttf"), &game->font))
		Platform_ExitWithErrorMessage(Str("Could not load font './assets/FalstinRegular-XOr2.ttf'."));
	if (!engine->render->load_texture_from_file(Str("./assets/base_texture.png"), &game->base_texture))
		Platform_ExitWithErrorMessage(Str("Could not load sprites from file './assets/base_texture.png'."));
	
	game->player = (Game_Player) {
		.pos = { 100.0f, 100.0f },
		.life = 5,
	};
	
	// Discord
	const int64 appid = 778719957956689922LL;
	
	game->discord.activity = (struct DiscordActivity) {
		.assets = {
			.large_image = "eredin",
			.large_text = "cat",
			.small_image = "preto",
			.small_text = "white",
		},
		
		.application_id = appid,
		.type = DiscordActivityType_Playing,
		.name = "Name",
		.state = "State",
		.details = "Details",
	};
	
	if (PlsDiscord_Init(appid, &game->discord))
		PlsDiscord_UpdateActivity(&game->discord);
}

static void
Game_UpdateDiscordEarly(void)
{
	void* scratch_arena_save = Arena_End(engine->scratch_arena);
	
	if (game->discord.connected)
	{
		PlsDiscord_Client* discord = &game->discord;
		PlsDiscord_Event* event = NULL;
		PlsDiscord_EarlyUpdate(discord, engine->scratch_arena, &event);
		
		for (; event; event = event->next)
		{
			enum EDiscordResult result = 0;
			
			switch (event->kind)
			{
				case PlsDiscord_EventKind_OnCurrentUserUpdate:
				{
					if (event->on_current_user_update.result != DiscordResult_Ok)
					{
						Platform_DebugLog("Discord: OnCurrentUserUpdate event failed.");
						break;
					}
					
					struct DiscordUser user = event->on_current_user_update.state;
					
					Platform_DebugLog("UserID: %I\tUsername: %s#%.*s\n", user.id, user.username, 4, user.discriminator);
				} break;
				
				case PlsDiscord_EventKind_CreateLobby:
				{
					if (event->create_lobby.result != DiscordResult_Ok)
					{
						Platform_DebugLog("Discord: CreateLobby event failed.");
						break;
					}
					
					result |= PlsDiscord_UpdateLobbyActivity(discord);
					result |= PlsDiscord_ConnectNetwork(discord);
				} break;
				
				case PlsDiscord_EventKind_ConnectLobbyWithActivitySecret:
				{
					if (event->connect_lobby_with_activity_secret.result != DiscordResult_Ok)
					{
						Platform_DebugLog("Discord: ConnectLobbyWithActivitySecret event failed.");
						break;
					}
					
					result |= PlsDiscord_UpdateLobbyActivity(discord);
					result |= PlsDiscord_ConnectNetwork(discord);
				} break;
				
				case PlsDiscord_EventKind_OnLobbyUpdate:
				{
					result |= PlsDiscord_UpdateLobbyActivity(discord);
				} break;
				
				case PlsDiscord_EventKind_OnActivityJoin:
				{
					PlsDiscord_JoinLobby(discord, event->on_activity_join.secret);
				} break;
				
				case PlsDiscord_EventKind_OnNetworkMessage:
				{
					String data = StrMake(event->on_network_message.data_length, event->on_network_message.data);
					Platform_DebugMessageBox("Message: %.*s", StrFmt(data));
				} break;
				
				default:
				{
					Platform_DebugLog("Discord: Event %i\n", event->kind);
				} break;
			}
			
			Assert(result == 0);
		}
	}
	
	Arena_Pop(engine->scratch_arena, scratch_arena_save);
}

static void
Game_UpdateAndRender(void)
{
	Trace();
	
	//- Update
	if (engine->platform->window_should_close)
		engine->running = false;
	
	Game_UpdateDiscordEarly();
	
	if (Engine_IsPressed(engine->input->keyboard, 'O'))
		PlsDiscord_CreateLobby(&game->discord);
	if (Engine_IsPressed(engine->input->keyboard, 'P'))
		PlsDiscord_ConnectNetwork(&game->discord);
	if (Engine_IsPressed(engine->input->keyboard, 'I'))
	{
		int64 userid = 0;
		
		for (int32 i = 0; i < 2; ++i)
		{
			int64 id;
			if (game->discord.lobbies->get_member_user_id(game->discord.lobbies, game->discord.lobby.id, i, &id) != DiscordResult_Ok)
				break;
			
			if (id != game->discord.user.id)
			{
				userid = id;
				break;
			}
		}
		
		Platform_DebugMessageBox("%I", userid);
		
		if (userid)
		{
			int32 r = PlsDiscord_SendNetworkMessage(&game->discord, userid, true, Str("oi."));
			Platform_DebugMessageBox("%i", r);
		}
	}
	
	vec2 mouse_pos;
	vec4 camera_bbox;
	
	Game_CameraBoundingBox(camera_bbox);
	
	mouse_pos[0] = camera_bbox[0] + engine->input->mouse.pos[0] / engine->platform->window_width  * camera_bbox[2];
	mouse_pos[1] = camera_bbox[1] + engine->input->mouse.pos[1] / engine->platform->window_height * camera_bbox[3];
	
	// NOTE(ljre): Update Player
	{
		const float32 player_speed = 3.0f;
		vec2 move = {
			Engine_IsDown(engine->input->keyboard, 'D') - Engine_IsDown(engine->input->keyboard, 'A'),
			Engine_IsDown(engine->input->keyboard, 'S') - Engine_IsDown(engine->input->keyboard, 'W'),
		};
		
		game->player.speed[0] = glm_lerp(game->player.speed[0], move[0] * player_speed, 0.3f * engine->delta_time);
		game->player.speed[1] = glm_lerp(game->player.speed[1], move[1] * player_speed, 0.3f * engine->delta_time);
		
		game->player.pos[0] += game->player.speed[0] * engine->delta_time;
		game->player.pos[1] += game->player.speed[1] * engine->delta_time;
		
		if (Engine_IsPressed(engine->input->mouse, Engine_MouseButton_Left))
		{
			vec2 dir = {
				mouse_pos[0] - game->player.pos[0],
				mouse_pos[1] - game->player.pos[1],
			};
			
			glm_vec2_normalize(dir);
			
			float32 angle = atan2f(dir[1], dir[0]);
			
			Game_Bullet bullet = {
				.kind = Game_BulletKind_Normal,
				.pos = {
					game->player.pos[0] + dir[0] * 5.0f,
					game->player.pos[1] + dir[1] * 5.0f,
				},
				.speed = {
					dir[0] * 10.0f,
					dir[1] * 10.0f,
				},
				.angle = angle,
			};
			
			Game_PushFriendlyBullet(&bullet);
		}
		
		for (int32 i = 0; i < game->friendly_bullet_count; ++i)
		{
			Game_Bullet* bullet = &game->friendly_bullets[i];
			
			bullet->pos[0] += bullet->speed[0] * engine->delta_time;
			bullet->pos[1] += bullet->speed[1] * engine->delta_time;
			
			// NOTE(ljre): Check if bullet is outside the camera
			const float32 size = 4.0f;
			
			vec4 bullet_bbox = {
				bullet->pos[0] - size*0.5f,
				bullet->pos[1] - size*0.5f,
				size,
				size
			};
			
			if (!Game_Aabb(bullet_bbox, camera_bbox))
			{
				*bullet = game->friendly_bullets[--game->friendly_bullet_count];
				--i;
				continue;
			}
		}
	}
	
	//- Render
	Render_ClearColor(engine, (vec4) { 0.1f, 0.15f, 0.3f, 1.0f });
	
	// NOTE(ljre): Draw Sprites
	{
		Arena* arena = engine->scratch_arena;
		void* saved_state = Arena_End(arena);
		Render_Data2DInstance* sprites = Arena_EndAligned(arena, 16);
		Render_Data2DInstance* spr;
		
		Render_Camera2D camera = {
			.pos = { 0 },
			.size = { engine->platform->window_width, engine->platform->window_height },
			.zoom = global_camera_zoom,
		};
		
		// NOTE(ljre): Draw Player
		{
			spr = Arena_Push(arena, sizeof(*spr));
			
			glm_mat4_identity(spr->transform);
			glm_translate(spr->transform, (vec3) { game->player.pos[0], game->player.pos[1] });
			glm_scale(spr->transform, (vec3) { 16.0f, 16.0f, 1.0f });
			glm_translate(spr->transform, (vec3) { -0.5f, -0.5f });
			
			spr->texcoords[0] = 16.0f / 128.0f;
			spr->texcoords[1] = 0.0f;
			spr->texcoords[2] = 16.0f / 128.0f;
			spr->texcoords[3] = 16.0f / 128.0f;
			
			spr->color[0] = 1.0f;
			spr->color[1] = 1.0f;
			spr->color[2] = 1.0f;
			spr->color[3] = 1.0f;
		}
		
		// NOTE(ljre): Draw Bullets
		for (int32 i = 0; i < game->friendly_bullet_count; ++i)
		{
			Game_Bullet* bullet = &game->friendly_bullets[i];
			spr = Arena_Push(arena, sizeof(*spr));
			
			glm_mat4_identity(spr->transform);
			glm_translate(spr->transform, (vec3) { bullet->pos[0], bullet->pos[1] });
			glm_scale(spr->transform, (vec3) { 4.0f, 4.0f, 1.0f });
			glm_rotate(spr->transform, bullet->angle, (vec3) { 0.0f, 0.0f, 1.0f });
			glm_translate(spr->transform, (vec3) { -0.5f, -0.5f });
			
			spr->texcoords[0] = 0.0f;
			spr->texcoords[1] = 0.0f;
			spr->texcoords[2] = 16.0f / 128.0f;
			spr->texcoords[3] = 16.0f / 128.0f;
			
			spr->color[0] = 1.0f;
			spr->color[1] = 1.0f;
			spr->color[2] = 1.0f;
			spr->color[3] = 1.0f;
		}
		
		Render_Data2D render_data = {
			.texture = &game->base_texture,
			.camera = camera,
			.instance_count = (Render_Data2DInstance*)Arena_End(arena) - sprites,
			.instances = sprites,
		};
		
		Render_Draw2D(engine, &render_data);
		
		Arena_Pop(arena, saved_state);
	}
	
	String text = String_PrintfLocal(128, "mouse (%.2f, %.2f)\ncamera (%.2f, %.2f, %.2f, %.2f)\n",
		mouse_pos[0], mouse_pos[1],
		camera_bbox[0], camera_bbox[1], camera_bbox[2], camera_bbox[3]);
	engine->render->draw_text(&game->font, text, (vec3) { 10.0f, 10.0f }, 32.0f, GLM_VEC4_ONE, (vec3) { 0 });
	
	PlsDiscord_LateUpdate(&game->discord);
	Engine_FinishFrame();
}

API void
Game_Main(Engine_Data* data)
{
	Trace();
	
	char buf[128];
	String_PrintfBuffer(buf, sizeof(buf), "0x%8x", 0xEFEFEu);
	
	engine = data;
	game = data->game;
	Arena_Clear(engine->scratch_arena);
	
	if (!engine->game)
	{
		engine->game = game = Arena_Push(engine->persistent_arena, sizeof(Game_Data));
		Game_Init();
	}
	
	Game_UpdateAndRender();
}
