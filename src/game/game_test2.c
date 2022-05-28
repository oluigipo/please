internal Engine_Data* engine;
internal Game_Data* game;

internal const float32 global_camera_zoom = 4.0f;

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
};

internal void
Game_PushFriendlyBullet(const Game_Bullet* bullet)
{
	Assert(game->friendly_bullet_count < ArrayLength(game->friendly_bullets));
	
	game->friendly_bullets[game->friendly_bullet_count++] = *bullet;
}

internal void
Game_CameraBoundingBox(vec4 out_bbox)
{
	float32 width = Platform_WindowWidth();
	float32 height = Platform_WindowWidth();
	float32 inv_zoom = 1.0f / global_camera_zoom;
	
	out_bbox[0] = -width  * inv_zoom * 0.5f;
	out_bbox[1] = -height * inv_zoom * 0.5f;
	out_bbox[2] =  width  * inv_zoom;
	out_bbox[3] =  height * inv_zoom;
}

//~ Main
internal void
Game_Init(void)
{
	Trace();
	
	MemSet(game, 0, sizeof(*game));
	
	if (!engine->renderer->load_font_from_file(Str("./assets/FalstinRegular-XOr2.ttf"), &game->font))
		Platform_ExitWithErrorMessage(Str("Could not load font './assets/FalstinRegular-XOr2.ttf'."));
	if (!engine->renderer->load_texture_from_file(Str("./assets/base_texture.png"), &game->base_texture))
		Platform_ExitWithErrorMessage(Str("Could not load sprites from file './assets/base_texture.png'."));
	
	game->player = (Game_Player) {
		.pos = { 100.0f, 100.0f },
		.life = 5,
	};
}

internal void
Game_UpdateAndRender(void)
{
	Trace();
	
	//- Update
	if (Platform_WindowShouldClose())
		engine->running = false;
	
	Engine_MouseState mouse = engine->input->mouse;
	
	vec2 mouse_pos;
	vec4 camera_bbox;
	
	Game_CameraBoundingBox(camera_bbox);
	
	mouse_pos[0] = camera_bbox[0] + mouse.pos[0] / Platform_WindowWidth()  * camera_bbox[2];
	mouse_pos[1] = camera_bbox[1] + mouse.pos[1] / Platform_WindowHeight() * camera_bbox[3];
	
	// NOTE(ljre): Update Player
	{
		const float32 player_speed = 3.0f;
		vec2 move = {
			Engine_IsDown(engine->input->keyboard, 'D') - Engine_IsDown(engine->input->keyboard, 'A'),
			Engine_IsDown(engine->input->keyboard, 'S') - Engine_IsDown(engine->input->keyboard, 'W'),
		};
		
		game->player.speed[0] = glm_lerp(game->player.speed[0], move[0] * player_speed, 0.3f);
		game->player.speed[1] = glm_lerp(game->player.speed[1], move[1] * player_speed, 0.3f);
		
		game->player.pos[0] += game->player.speed[0];
		game->player.pos[1] += game->player.speed[1];
		
		if (Engine_IsPressed(mouse, Engine_MouseButton_Left))
		{
			vec2 dir = {
				mouse_pos[0] - game->player.pos[0],
				mouse_pos[1] - game->player.pos[1],
			};
			
			glm_vec2_normalize(dir);
			
			Game_Bullet bullet = {
				.kind = Game_BulletKind_Normal,
				.pos = { game->player.pos[0], game->player.pos[1] },
				.speed = {
					dir[0] * 10.0f,
					dir[1] * 10.0f,
				},
			};
			
			Game_PushFriendlyBullet(&bullet);
		}
	}
	
	//- Render
	engine->renderer->begin();
	engine->renderer->clear_background(0.1f, 0.15f, 0.3f, 1.0f);
	
	// NOTE(ljre): Draw Sprites
	{
		Arena* arena = engine->temp_arena;
		void* saved_state = Arena_End(arena);
		Engine_Renderer2DSprite* sprites = Arena_EndAligned(arena, 15);
		Engine_Renderer2DSprite* spr;
		
		Engine_RendererCamera camera = {
			.pos = { 0 },
			.size = { Platform_WindowWidth(), Platform_WindowHeight() },
			.zoom = 4.0f,
		};
		
		// NOTE(ljre): Draw Player
		{
			spr = Arena_Push(arena, sizeof(*spr));
			
			glm_mat4_identity(spr->transform);
			glm_translate(spr->transform, (vec3) { game->player.pos[0], game->player.pos[1] });
			glm_scale(spr->transform, (vec3) { 16.0f, 16.0f, 1.0f });
			
			spr->texcoords[0] = 1.0f / 128.0f * 16.0f;
			spr->texcoords[1] = 0.0f;
			spr->texcoords[2] = 1.0f / 128.0f * 16.0f;
			spr->texcoords[3] = 1.0f / 128.0f * 16.0f;
			
			spr->color[0] = 1.0f;
			spr->color[1] = 1.0f;
			spr->color[2] = 1.0f;
			spr->color[3] = 1.0f;
		}
		
		Engine_Renderer2DLayer layer = {
			.texture = &game->base_texture,
			.sprite_count = (Engine_Renderer2DSprite*)Arena_End(arena) - sprites,
			.sprites = sprites,
		};
		
		engine->renderer->draw_2dlayer(&layer, &camera);
		
		Arena_Pop(arena, saved_state);
	}
	
	char buf[128];
	uintsize len = SPrintf(buf, sizeof(buf), "mouse (%.2f, %.2f)", mouse_pos[0], mouse_pos[1]);
	engine->renderer->draw_text(&game->font, StrMake(len, buf), (vec3) { 10.0f, 10.0f }, 32.0f, GLM_VEC4_ONE, (vec3) { 0 });
	
	Engine_FinishFrame();
}

API void
Game_Main(Engine_Data* data)
{
	Trace();
	
	engine = data;
	game = data->game;
	Arena_Clear(engine->temp_arena);
	
	if (!engine->game)
	{
		engine->game = game = Arena_PushDirty(engine->persistent_arena, sizeof(Game_Data));
		Game_Init();
	}
	
	Game_UpdateAndRender();
}
