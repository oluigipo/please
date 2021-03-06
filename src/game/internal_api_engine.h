#ifndef INTERNAL_API_ENGINE_H
#define INTERNAL_API_ENGINE_H

//~ Main Data
struct Engine_Data typedef Engine_Data;
struct Engine_RendererApi typedef Engine_RendererApi;
struct Engine_InputData typedef Engine_InputData;

struct Game_Data typedef Game_Data;
struct Platform_GraphicsContext typedef Platform_GraphicsContext;

struct Engine_Data
{
	Arena* persistent_arena;
	Arena* temp_arena;
	Game_Data* game;
	
	const Platform_GraphicsContext* graphics_context;
	const Engine_RendererApi* renderer;
	Engine_InputData* input;
	
	float32 delta_time;
	float64 last_frame_time;
	
	bool8 outputed_sound_this_frame;
	bool8 running;
};

// Engine entry point. It shall be called by the platform layer.
API int32 Engine_Main(int32 argc, char** argv);
API void Engine_FinishFrame(void);
API void Game_Main(Engine_Data* data);

//~ Audio
struct Engine_PlayingAudio
{
	const Asset_SoundBuffer* sound;
	int32 frame_index; // if < 0, then it will start playing at '-frame_index - 1'
	bool32 loop;
	float32 volume;
	float32 speed;
}
typedef Engine_PlayingAudio;

API bool32 Engine_LoadSoundBuffer(String path, Asset_SoundBuffer* out_sound);
API void Engine_FreeSoundBuffer(Asset_SoundBuffer* sound);
// NOTE(ljre): Should be called once per frame.
API void Engine_PlayAudios(Engine_PlayingAudio* audios, int32* audio_count, float32 volume);

//~ Input
struct Engine_ButtonState
{
	uint8 changes: 7;
	bool is_down: 1;
}
typedef Engine_ButtonState;

static_assert(sizeof(Engine_ButtonState) == 1);

enum Engine_KeyboardKey
{
	Engine_KeyboardKey_Any = 0,
	
	Engine_KeyboardKey_Escape,
	Engine_KeyboardKey_Up, Engine_KeyboardKey_Down, Engine_KeyboardKey_Left, Engine_KeyboardKey_Right,
	Engine_KeyboardKey_LeftControl, Engine_KeyboardKey_RightControl, Engine_KeyboardKey_Control,
	Engine_KeyboardKey_LeftShift, Engine_KeyboardKey_RightShift, Engine_KeyboardKey_Shift,
	Engine_KeyboardKey_LeftAlt, Engine_KeyboardKey_RightAlt, Engine_KeyboardKey_Alt,
	Engine_KeyboardKey_Tab, Engine_KeyboardKey_Enter, Engine_KeyboardKey_Backspace,
	Engine_KeyboardKey_PageUp, Engine_KeyboardKey_PageDown, Engine_KeyboardKey_End, Engine_KeyboardKey_Home,
	// NOTE(ljre): Try to not mess up. Engine_KeyboardKey_Space = 32, and those above may reach it if you blindly add stuff.
	
	Engine_KeyboardKey_Space = ' ',
	
	Engine_KeyboardKey_0 = '0',
	Engine_KeyboardKey_1, Engine_KeyboardKey_2, Engine_KeyboardKey_3, Engine_KeyboardKey_4,
	Engine_KeyboardKey_5, Engine_KeyboardKey_6, Engine_KeyboardKey_7, Engine_KeyboardKey_8,
	Engine_KeyboardKey_9,
	
	Engine_KeyboardKey_A = 'A',
	Engine_KeyboardKey_B, Engine_KeyboardKey_C, Engine_KeyboardKey_D, Engine_KeyboardKey_E,
	Engine_KeyboardKey_F, Engine_KeyboardKey_G, Engine_KeyboardKey_H, Engine_KeyboardKey_I,
	Engine_KeyboardKey_J, Engine_KeyboardKey_K, Engine_KeyboardKey_L, Engine_KeyboardKey_M,
	Engine_KeyboardKey_N, Engine_KeyboardKey_O, Engine_KeyboardKey_P, Engine_KeyboardKey_Q,
	Engine_KeyboardKey_R, Engine_KeyboardKey_S, Engine_KeyboardKey_T, Engine_KeyboardKey_U,
	Engine_KeyboardKey_V, Engine_KeyboardKey_W, Engine_KeyboardKey_X, Engine_KeyboardKey_Y,
	Engine_KeyboardKey_Z,
	
	Engine_KeyboardKey_Numpad0,
	Engine_KeyboardKey_Numpad1, Engine_KeyboardKey_Numpad2, Engine_KeyboardKey_Numpad3, Engine_KeyboardKey_Numpad4,
	Engine_KeyboardKey_Numpad5, Engine_KeyboardKey_Numpad6, Engine_KeyboardKey_Numpad7, Engine_KeyboardKey_Numpad8,
	Engine_KeyboardKey_Numpad9,
	
	Engine_KeyboardKey_F1,
	Engine_KeyboardKey_F2,  Engine_KeyboardKey_F3,  Engine_KeyboardKey_F4,  Engine_KeyboardKey_F5,
	Engine_KeyboardKey_F6,  Engine_KeyboardKey_F7,  Engine_KeyboardKey_F8,  Engine_KeyboardKey_F9,
	Engine_KeyboardKey_F10, Engine_KeyboardKey_F11, Engine_KeyboardKey_F12, Engine_KeyboardKey_F13,
	Engine_KeyboardKey_F14,
	
	Engine_KeyboardKey_Count,
}
typedef Engine_KeyboardKey;

struct Engine_KeyboardState
{
	int32 buffered_count;
	
	Engine_ButtonState buttons[Engine_KeyboardKey_Count];
	uint8 buffered[128];
}
typedef Engine_KeyboardState;

enum Engine_MouseButton
{
	Engine_MouseButton_Left,
	Engine_MouseButton_Middle,
	Engine_MouseButton_Right,
	Engine_MouseButton_Other0,
	Engine_MouseButton_Other1,
	Engine_MouseButton_Other2,
	
	Engine_MouseButton_Count,
}
typedef Engine_MouseButton;

struct Engine_MouseState
{
	// NOTE(ljre): Position relative to window.
	float32 pos[2];
	float32 old_pos[2];
	int32 scroll;
	
	Engine_ButtonState buttons[Engine_MouseButton_Count];
}
typedef Engine_MouseState;

enum Engine_GamepadButton
{
	Engine_GamepadButton_A,
	Engine_GamepadButton_B,
	Engine_GamepadButton_X,
	Engine_GamepadButton_Y,
	Engine_GamepadButton_Left,
	Engine_GamepadButton_Right,
	Engine_GamepadButton_Up,
	Engine_GamepadButton_Down,
	Engine_GamepadButton_LB,
	Engine_GamepadButton_RB,
	Engine_GamepadButton_RS,
	Engine_GamepadButton_LS,
	Engine_GamepadButton_Start,
	Engine_GamepadButton_Back,
	
	Engine_GamepadButton_Count,
}
typedef Engine_GamepadButton;

struct Engine_GamepadState
{
	// [0] = left < 0 < right
	// [1] = up < 0 < down
	float32 left[2];
	float32 right[2];
	
	// 0..1
	float32 lt, rt;
	
	Engine_ButtonState buttons[Engine_GamepadButton_Count];
}
typedef Engine_GamepadState;

#define Engine_MAX_GAMEPAD_COUNT 16

struct Engine_InputData
{
	Engine_KeyboardState keyboard;
	Engine_MouseState mouse;
	Engine_GamepadState gamepads[Engine_MAX_GAMEPAD_COUNT];
	
	uint16 connected_gamepads;
};

// NOTE(ljre): Helper macros.
//             Returns a bool.
//
//             state: Engine_GamepadState, Engine_MouseState, or Engine_KeyboardState.
//             btn: A button enum.
#define Engine_IsPressed(state, btn) ((state).buttons[btn].is_down && (state).buttons[btn].changes & 1)
#define Engine_IsDown(state, btn) ((state).buttons[btn].is_down)
#define Engine_IsReleased(state, btn) (!(state).buttons[btn].is_down && (state).buttons[btn].changes & 1)
#define Engine_IsUp(state, btn) (!(state).buttons[btn].is_down)

API bool Engine_IsGamepadConnected(uint32 index);
API int32 Engine_ConnectedGamepadCount(void);
API int32 Engine_ConnectedGamepadsIndices(int32 out_indices[Engine_MAX_GAMEPAD_COUNT]);

//~ Renderer
struct Engine_RendererCamera
{
	vec3 pos;
	
	union
	{
		// NOTE(ljre): 2D Stuff
		struct { vec2 size; float32 zoom; float32 angle; };
		
		// NOTE(ljre): 3D Stuff
		struct { vec3 dir; vec3 up; };
	};
}
typedef Engine_RendererCamera;

struct Engine_Renderer3DPointLight
{
	vec3 position;
	
	float32 constant, linear, quadratic;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
}
typedef Engine_Renderer3DPointLight;

struct Engine_Renderer3DFlashlight
{
	vec3 position;
	vec3 direction;
	vec3 color;
	
	float32 constant, linear, quadratic;
	
	float32 inner_cutoff;
	float32 outer_cutoff;
}
typedef Engine_Renderer3DFlashlight;

struct Engine_Renderer3DModel
{
	mat4 transform;
	Asset_3DModel* asset;
	vec4 color;
}
typedef Engine_Renderer3DModel;

struct Engine_Renderer3DScene
{
	Asset_3DModel* light_model;
	vec3 dirlight;
	vec3 dirlight_color;
	
	uint32 shadow_fbo, shadow_depthmap;
	uint32 gbuffer, gbuffer_pos, gbuffer_norm, gbuffer_albedo, gbuffer_depth;
	
	int32 model_count;
	int32 point_light_count;
	int32 flashlights_count;
	
	Engine_Renderer3DModel* models;
	Engine_Renderer3DPointLight* point_lights;
	Engine_Renderer3DFlashlight* flashlights;
}
typedef Engine_Renderer3DScene;

struct Engine_Renderer2DSprite
{
	mat4 transform;
	vec4 texcoords; // [left, top, width, height] normalized
	vec4 color;
}
typedef Engine_Renderer2DSprite;

struct Engine_Renderer2DLayer
{
	Asset_Texture* texture;
	
	uintsize sprite_count;
	Engine_Renderer2DSprite* sprites;
}
typedef Engine_Renderer2DLayer;

API void Engine_CalcViewMatrix2D(const Engine_RendererCamera* camera, mat4 out_view);
API void Engine_CalcViewMatrix3D(const Engine_RendererCamera* camera, mat4 out_view, float32 fov, float32 aspect);
API void Engine_CalcModelMatrix2D(const vec2 pos, const vec2 scale, float32 angle, mat4 out_view);
API void Engine_CalcModelMatrix3D(const vec3 pos, const vec3 scale, const vec3 rot, mat4 out_view);
API void Engine_CalcPointInCamera2DSpace(const Engine_RendererCamera* camera, const vec2 pos, vec2 out_pos);

struct Engine_RendererApi
{
	bool (*load_font_from_file)(String path, Asset_Font* out_font);
	bool (*load_3dmodel_from_file)(String path, Asset_3DModel* out_model);
	bool (*load_texture_from_file)(String path, Asset_Texture* out_texture);
	bool (*load_texture_array_from_file)(String path, Asset_Texture* out_texture, int32 cell_width, int32 cell_height);
	
	void (*clear_background)(float32 r, float32 g, float32 b, float32 a);
	void (*begin)(void);
	void (*draw_rectangle)(vec4 color, vec3 pos, vec3 size, vec3 alignment);
	void (*draw_texture)(const Asset_Texture* texture, const mat4 transform, const mat4 view, const vec4 color);
	void (*draw_text)(const Asset_Font* font, String text, const vec3 pos, float32 char_height, const vec4 color, const vec3 alignment);
	
	void (*draw_3dscene)(Engine_Renderer3DScene* scene, const Engine_RendererCamera* camera);
	void (*draw_2dlayer)(const Engine_Renderer2DLayer* layer, const Engine_RendererCamera* camera);
};

#endif //INTERNAL_API_ENGINE_H
