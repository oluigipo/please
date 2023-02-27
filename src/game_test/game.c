#include "api_engine.h"
#include "api_steam.h"
#include "util_random.h"
#include "util_json.h"
#include "util_gltf.h"
#include "util_debugui.h"

#ifdef CONFIG_ENABLE_EMBED
IncludeBinary(g_music_ogg, "music.ogg");
IncludeBinary(g_luigi_ogg, "luigi.ogg");
//IncludeBinary(g_font_ttf, "C:/Windows/Fonts/Arial.ttf");
//IncludeBinary(g_corset_model, "assets/corset.glb");
#endif

static G_GlobalData* game;
static E_GlobalData* engine;

struct G_SnakeState typedef G_SnakeState;
struct G_Scene3DState typedef G_Scene3DState;
struct G_StressState typedef G_StressState;

enum G_GlobalState
{
	G_GlobalState_MainMenu,
	G_GlobalState_Snake,
	G_GlobalState_Scene3D,
	G_GlobalState_Stress,
}
typedef G_GlobalState;

struct G_GlobalData
{
	G_GlobalState state;
	G_GlobalState previous_state;
	E_Font font;
	Arena_Savepoint persistent_arena_save;
	URng_State rng;
	
	E_SoundHandle music;
	E_SoundHandle sound_luigi;
	
	union
	{
		G_SnakeState* snake;
		G_Scene3DState* scene3d;
		G_StressState* stress;
	};
};

#include "game_snake.c"
#include "game_scene3d.c"
#include "game_stress.c"

//~ NOTE(ljre): Main menu implementation
static void
G_Init(void)
{
	Trace();
	
#ifdef CONFIG_ENABLE_STEAM
	if (S_Init(0))
		OS_DebugLog("Steam user nickname: %S\n", S_GetUserNickname());
#endif
	
	game = engine->game = Arena_PushStruct(engine->persistent_arena, G_GlobalData);
	game->previous_state = game->state = G_GlobalState_MainMenu;
	
	URng_Init(&game->rng, OS_CurrentPosixTime());
	
	E_FontDesc desc = {
		.arena = engine->persistent_arena,
		.ttf = { 0 },
		
		.char_height = 32.0f,
		.prebake_ranges = {
			{ 0x21, 0x7E },
			{ 0xA1, 0xFF },
		},
	};
	
	//#ifdef CONFIG_ENABLE_EMBED
	//desc.ttf = BufRange(g_font_ttf_begin, g_font_ttf_end);
	//#else
	SafeAssert(OS_ReadEntireFile(Str("C:/Windows/Fonts/Arial.ttf"), engine->persistent_arena, (void**)&desc.ttf.data, &desc.ttf.size));
	//SafeAssert(OS_ReadEntireFile(Str("C:/Users/luigi/AppData/Local/Microsoft/Windows/Fonts/PxPlus_IBM_VGA8 (1).ttf"), engine->persistent_arena, (void**)&desc.ttf.data, &desc.ttf.size));
	//#endif
	SafeAssert(E_MakeFont(&desc, &game->font));
	
	{
		Buffer ogg;
		bool ok;
		
#ifdef CONFIG_ENABLE_EMBED
		ogg = BufRange(g_music_ogg_begin, g_music_ogg_end);
		ok = true;
#else
		ok = OS_ReadEntireFile(Str("music.ogg"), engine->persistent_arena, (void**)&ogg.data, &ogg.size);
#endif
		
		if (ok)
		{
			SafeAssert(E_LoadSound(ogg, &game->music, NULL));
			SafeAssert(E_PlaySound(game->music, &(E_PlaySoundOptions) { .volume = 0.5f }, NULL));
		}
		
#ifdef CONFIG_ENABLE_EMBED
		ogg = BufRange(g_luigi_ogg_begin, g_luigi_ogg_end);
		ok = true;
#else
		ok = OS_ReadEntireFile(Str("luigi.ogg"), engine->persistent_arena, (void**)&ogg.data, &ogg.size);
#endif
		
		if (ok)
			SafeAssert(E_LoadSound(ogg, &game->sound_luigi, NULL));
	}
	
	game->persistent_arena_save = Arena_Save(engine->persistent_arena);
}

static bool
G_MenuButton(E_RectBatch* batch, float32* inout_y, String text)
{
	Trace();
	
	const float32 button_width = 300.0f;
	const float32 button_height = 100.0f;
	const float32 button_x = (engine->os->window.width - button_width) / 2;
	const float32 mouse_x = engine->os->input.mouse.pos[0];
	const float32 mouse_y = engine->os->input.mouse.pos[1];
	
	float32 button_y = *inout_y;
	
	bool is_mouse_pressing = OS_IsDown(engine->os->input.mouse, OS_MouseButton_Left);
	bool is_mouse_over = true
		&& (mouse_x >= button_x && button_x + button_width  >= mouse_x)
		&& (mouse_y >= button_y && button_y + button_height >= mouse_y);
	
	vec4 color = { 0.3f, 0.3f, 0.3f, 1.0f };
	
	if (is_mouse_over)
	{
		if (is_mouse_pressing)
			color[0] = color[1] = color[2] = 0.2f;
		else
			color[0] = color[1] = color[2] = 0.4f;
	}
	
	E_PushRect(batch, &(E_RectBatchElem) {
		.pos = { button_x, button_y },
		.scaling[0][0] = button_width,
		.scaling[1][1] = button_height,
		.tex_kind = 3.0f,
		.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
		.color = { color[0], color[1], color[2], color[3] },
	});
	
	float32 scale = sinf((float32)OS_GetTimeInSeconds()) * 0.2f + 1.5f;
	
	E_PushText(batch, &game->font, text, vec2(button_x, button_y + button_height*0.25f), vec2(scale, scale), GLM_VEC4_ONE);
	
	button_y += button_height * 1.5f;
	
	*inout_y = button_y;
	return is_mouse_over && OS_IsReleased(engine->os->input.mouse, OS_MouseButton_Left);
}

static void
G_UpdateAndRender(void)
{
	Trace();
	
#ifdef CONFIG_ENABLE_STEAM
	S_Update();
#endif
	
	if (engine->os->window.should_close || OS_IsPressed(engine->os->input.keyboard, OS_KeyboardKey_Escape))
		engine->running = false;
	
	if (OS_IsPressed(engine->os->input.keyboard, OS_KeyboardKey_Space) && E_IsValidSoundHandle(game->sound_luigi))
		E_PlaySound(game->sound_luigi, &(E_PlaySoundOptions) { .volume = 0.5f }, NULL);
	
	E_DrawClear(0.2f, 0.0f, 0.4f, 1.0f);
	
	if (game->previous_state != game->state)
	{
		switch (game->state)
		{
			case G_GlobalState_MainMenu: Arena_Restore(game->persistent_arena_save); break;
			case G_GlobalState_Snake: G_SnakeInit(); break;
			case G_GlobalState_Scene3D: G_Scene3DInit(); break;
			case G_GlobalState_Stress: G_StressInit(); break;
		}
		
		game->previous_state = game->state;
	}
	
	switch (game->state)
	{
		case G_GlobalState_MainMenu:
		{
			float32 ui_y = 100.0f;
			E_RectBatch batch = {
				.arena = engine->frame_arena,
				.textures[0] = NULL,
				.textures[1] = &game->font.texture,
				.count = 0,
				.elements = Arena_EndAligned(engine->frame_arena, alignof(E_RectBatchElem)),
			};
			
			//- Squares
			for (int32 i = 0; i < 100; ++i)
			{
				uint64 random = Hash_IntHash64(i);
				
				float32 time = (float32)OS_GetTimeInSeconds() * 0.25f + (float32)i;
				float32 xoffset = cosf(time * 5.0f) * 100.0f;
				float32 yoffset = sinf(time * 1.4f) * 100.0f;
				
				float32 x = (random & 511) - 256.0f + xoffset;
				float32 y = (random>>9 & 511) - 256.0f + yoffset;
				float32 angle = (random>>18 & 511) / 512.0f * (float32)Math_PI*2 + time;
				float32 size = 32.0f;
				vec3 color = {
					(random>>27 & 255) / 255.0f,
					(random>>35 & 255) / 255.0f,
					(random>>43 & 255) / 255.0f,
				};
				
				E_PushRect(&batch, &(E_RectBatchElem) {
					.pos = { engine->os->window.width*0.5f + x, engine->os->window.height*0.5f + y },
					.scaling = {
						{ size*cosf(angle), size*-sinf(angle) },
						{ size*sinf(angle), size* cosf(angle) },
					},
					.tex_index = 0.0f,
					.tex_kind = 3.0f,
					.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
					.color = { color[0], color[1], color[2], 1.0f },
				});
			}
			
			//- Buttons
			if (G_MenuButton(&batch, &ui_y, Str("Play Snake")))
				game->state = G_GlobalState_Snake;
			if (G_MenuButton(&batch, &ui_y, Str("Play Scene 3D")))
				game->state = G_GlobalState_Scene3D;
			if (G_MenuButton(&batch, &ui_y, Str("Play Stress")))
				game->state = G_GlobalState_Stress;
			if (G_MenuButton(&batch, &ui_y, Str("Quit")))
				engine->running = false;
			
			float32 scl = 0.5f;
			UDebugUI_State debugui = UDebugUI_Begin(engine, &batch, &game->font, vec2(50.0f, 50.0f), vec2(scl, scl));
			
			static bool unfolded = false;
			if (UDebugUI_PushFoldable(&debugui, Str("Options"), &unfolded))
			{
				if (UDebugUI_PushButton(&debugui, Str("Stop music")))
					E_StopAllSounds(&game->music);
				if (UDebugUI_PushButton(&debugui, Str("luigi")))
					E_PlaySound(game->sound_luigi, &(E_PlaySoundOptions) { 0 }, NULL);
				
				static bool caps_unfolded = false;
				if (UDebugUI_PushFoldable(&debugui, Str("RenderBackend Capabilities"), &caps_unfolded))
				{
					RB_Capabilities caps = { 0 };
					RB_QueryCapabilities(&caps);
					
					UDebugUI_PushTextF(&debugui, "API: %S\nDriver: %S\nMaxTextureSize: %i", caps.backend_api, caps.driver, caps.max_texture_size);
					
					UDebugUI_PopFoldable(&debugui);
				}
				
				static bool arenas_unfolded = false;
				if (UDebugUI_PushFoldable(&debugui, Str("Arenas Info"), &arenas_unfolded))
				{
					UDebugUI_PushArenaInfo(&debugui, engine->persistent_arena, Str("Persistent"));
					UDebugUI_PushArenaInfo(&debugui, engine->scratch_arena, Str("Scratch"));
					UDebugUI_PushArenaInfo(&debugui, engine->frame_arena, Str("Frame"));
					UDebugUI_PushArenaInfo(&debugui, engine->audio_thread_arena, Str("Audio Thread"));
					
					UDebugUI_PopFoldable(&debugui);
				}
				
				static bool audiodev_unfolded = false;
				if (UDebugUI_PushFoldable(&debugui, Str("Audio Devices"), &audiodev_unfolded))
				{
					UDebugUI_PushTextF(&debugui, "Current playing device ID: %u", engine->os->audio.current_device_id);
					UDebugUI_PushTextF(&debugui, "Sample Rate: %i", engine->os->audio.mix_sample_rate);
					UDebugUI_PushTextF(&debugui, "Channels: %i", engine->os->audio.mix_channels);
					UDebugUI_PushTextF(&debugui, "Frame Pull Rate: %i", engine->os->audio.mix_frame_pull_rate);
					UDebugUI_PushVerticalSpacing(&debugui, 24.0f);
					UDebugUI_PushTextF(&debugui, "Devices:");
					
					static bool devs_unfolded[32];
					for (int32 i = 0; i < Min(engine->os->audio.device_count, ArrayLength(devs_unfolded)); ++i)
					{
						const OS_AudioDeviceInfo* dev = &engine->os->audio.devices[i];
						String text = String_PrintfLocal(256, "ID %u", dev->id);
						
						if (UDebugUI_PushFoldable(&debugui, text, &devs_unfolded[i]))
						{
							UDebugUI_PushTextF(&debugui, "Name: %S\nDescription: %S\nInterface: %S", dev->name, dev->description, dev->interface_name);
							
							UDebugUI_PopFoldable(&debugui);
						}
					}
					
					UDebugUI_PopFoldable(&debugui);
				}
				
				UDebugUI_PopFoldable(&debugui);
			}
			
			UDebugUI_End(&debugui);
			
			E_DrawRectBatch(&batch, NULL);
		} break;
		
		case G_GlobalState_Snake: G_SnakeUpdateAndRender(); break;
		case G_GlobalState_Scene3D: G_Scene3DUpdateAndRender(); break;
		case G_GlobalState_Stress: G_StressUpdateAndRender(); break;
	}
	
	if (!engine->running)
	{
		E_UnloadSound(game->sound_luigi);
		E_UnloadSound(game->music);
	}
	
	E_FinishFrame();
}

API void
G_Main(E_GlobalData* data)
{
	Trace();
	
	engine = data;
	game = data->game;
	
	if (!game)
		G_Init();
	
	for Arena_TempScope(data->scratch_arena)
		G_UpdateAndRender();
}
