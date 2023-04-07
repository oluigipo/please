#include "os_internal_gamepad_map.h"

#define GAMEPAD_DEADZONE 0.3f

// TODO(ljre): it's probably better to move TranslateController and GamepadMappings to
//             the Engine instead of leaving it in the platform layer.

// TODO(ljre): Make more tests - I still don't trust it enough.

//~ Types and Macros
enum Win32_GamepadAPI
{
	Win32_GamepadAPI_None = 0,
	Win32_GamepadAPI_XInput = 1,
	Win32_GamepadAPI_DirectInput = 2,
}
typedef Win32_GamepadAPI;

struct Win32_Gamepad
{
	Win32_GamepadAPI api;
	bool connected;
	OS_GamepadState data;
	
	union
	{
		struct
		{
			DWORD index;
		} xinput;
		
		struct
		{
			IDirectInputDevice8W* device;
			GUID guid;
			
			int32 buttons[32];
			int32 button_count;
			int32 axes[16];
			int32 axis_count;
			int32 slider_count;
			int32 povs[8];
			int32 pov_count;
			
			const GamepadMappings* map;
		} dinput;
	};
}
typedef Win32_Gamepad;

#define XInputGetState global_proc_XInputGetState
#define XInputSetState global_proc_XInputSetState
#define XInputGetCapabilities global_proc_XInputGetCapabilities
#define DirectInput8Create global_proc_DirectInput8Create

typedef DWORD WINAPI ProcXInputGetState(DWORD index, XINPUT_STATE* state);
typedef DWORD WINAPI ProcXInputSetState(DWORD index, XINPUT_VIBRATION* vibration);
typedef DWORD WINAPI ProcXInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);
typedef HRESULT WINAPI ProcDirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);

#ifndef XINPUT_DEVSUBTYPE_UNKNOWN
#   define XINPUT_DEVSUBTYPE_UNKNOWN 0x00
#endif

#ifndef XINPUT_DEVSUBTYPE_FLIGHT_STICK
#   define XINPUT_DEVSUBTYPE_FLIGHT_STICK 0x04
#endif

#ifndef XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE
#   define XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE 0x07
#endif

#ifndef XINPUT_DEVSUBTYPE_GUITAR_BASS
#   define XINPUT_DEVSUBTYPE_GUITAR_BASS 0x0B
#endif

#ifndef XINPUT_DEVSUBTYPE_ARCADE_PAD
#   define XINPUT_DEVSUBTYPE_ARCADE_PAD 0x13
#endif

//~ Globals
static Win32_Gamepad   global_gamepads[OS_Limits_MaxGamepadCount];
static int32           global_gamepad_free[OS_Limits_MaxGamepadCount];
static int32           global_gamepad_free_count;
static IDirectInput8W* global_dinput;
static bool32          global_dinput_enabled;

static ProcXInputGetState*        XInputGetState;
static ProcXInputSetState*        XInputSetState;
static ProcXInputGetCapabilities* XInputGetCapabilities;
static ProcDirectInput8Create*    DirectInput8Create;

static const OS_KeyboardKey global_keyboard_key_table[] = {
	[0] = OS_KeyboardKey_Any,
	
	[VK_ESCAPE] = OS_KeyboardKey_Escape,
	[VK_CONTROL] = OS_KeyboardKey_Control,
	[VK_SHIFT] = OS_KeyboardKey_Shift,
	[VK_MENU] = OS_KeyboardKey_Alt,
	[VK_TAB] = OS_KeyboardKey_Tab,
	[VK_RETURN] = OS_KeyboardKey_Enter,
	[VK_BACK] = OS_KeyboardKey_Backspace,
	[VK_UP] = OS_KeyboardKey_Up,
	[VK_DOWN] = OS_KeyboardKey_Down,
	[VK_LEFT] = OS_KeyboardKey_Left,
	[VK_RIGHT] = OS_KeyboardKey_Right,
	[VK_LCONTROL] = OS_KeyboardKey_LeftControl,
	[VK_RCONTROL] = OS_KeyboardKey_RightControl,
	[VK_LSHIFT] = OS_KeyboardKey_LeftShift,
	[VK_RSHIFT] = OS_KeyboardKey_RightShift,
	[VK_PRIOR] = OS_KeyboardKey_PageUp,
	[VK_NEXT] = OS_KeyboardKey_PageDown,
	[VK_END] = OS_KeyboardKey_End,
	[VK_HOME] = OS_KeyboardKey_Home,
	
	[VK_SPACE] = OS_KeyboardKey_Space,
	
	['0'] = OS_KeyboardKey_0,
	OS_KeyboardKey_1, OS_KeyboardKey_2, OS_KeyboardKey_3, OS_KeyboardKey_4,
	OS_KeyboardKey_5, OS_KeyboardKey_6, OS_KeyboardKey_7, OS_KeyboardKey_8,
	OS_KeyboardKey_9,
	
	['A'] = OS_KeyboardKey_A,
	OS_KeyboardKey_B, OS_KeyboardKey_C, OS_KeyboardKey_D, OS_KeyboardKey_E,
	OS_KeyboardKey_F, OS_KeyboardKey_G, OS_KeyboardKey_H, OS_KeyboardKey_I,
	OS_KeyboardKey_J, OS_KeyboardKey_K, OS_KeyboardKey_L, OS_KeyboardKey_M,
	OS_KeyboardKey_N, OS_KeyboardKey_O, OS_KeyboardKey_P, OS_KeyboardKey_Q,
	OS_KeyboardKey_R, OS_KeyboardKey_S, OS_KeyboardKey_T, OS_KeyboardKey_U,
	OS_KeyboardKey_V, OS_KeyboardKey_W, OS_KeyboardKey_X, OS_KeyboardKey_Y,
	OS_KeyboardKey_Z,
	
	[VK_F1] = OS_KeyboardKey_F1,
	OS_KeyboardKey_F2, OS_KeyboardKey_F3,  OS_KeyboardKey_F4,  OS_KeyboardKey_F4,
	OS_KeyboardKey_F5, OS_KeyboardKey_F6,  OS_KeyboardKey_F7,  OS_KeyboardKey_F8,
	OS_KeyboardKey_F9, OS_KeyboardKey_F10, OS_KeyboardKey_F11, OS_KeyboardKey_F12,
	OS_KeyboardKey_F13,
	
	[VK_NUMPAD0] = OS_KeyboardKey_Numpad0,
	OS_KeyboardKey_Numpad1, OS_KeyboardKey_Numpad2, OS_KeyboardKey_Numpad3, OS_KeyboardKey_Numpad4,
	OS_KeyboardKey_Numpad5, OS_KeyboardKey_Numpad6, OS_KeyboardKey_Numpad7, OS_KeyboardKey_Numpad8,
	OS_KeyboardKey_Numpad9,
};

//~ Functions
#include <ext/guid_utils.h>

static inline bool
AreGUIDsEqual(const GUID* a, const GUID* b)
{
	return Mem_Compare(a, b, sizeof (GUID)) == 0;
}

static inline int32
GenGamepadIndex(void)
{
	return global_gamepad_free[--global_gamepad_free_count];
}

static inline void
ReleaseGamepadIndex(int32 index)
{
	global_gamepad_free[global_gamepad_free_count++] = index;
}

static inline void
NormalizeAxis(float32 axis[2])
{
	float32 len2 = axis[0]*axis[0] + axis[1]*axis[1];
	
	if (len2 < GAMEPAD_DEADZONE*GAMEPAD_DEADZONE)
	{
		axis[0] = 0.0f;
		axis[1] = 0.0f;
	}
	else
	{
		float32 invlen = 1.0f / sqrtf(len2);
		
		if (axis[0] < -0.05f || axis[0] > 0.05f)
			axis[0] *= invlen;
		else
			axis[0] = 0.0f;
		
		if (axis[1] < -0.05f || axis[1] > 0.05f)
			axis[1] *= invlen;
		else
			axis[1] = 0.0f;
	}
}

static DWORD WINAPI
XInputGetStateStub(DWORD index, XINPUT_STATE* state) { return ERROR_DEVICE_NOT_CONNECTED; }
static DWORD WINAPI
XInputSetStateStub(DWORD index, XINPUT_VIBRATION* state) { return ERROR_DEVICE_NOT_CONNECTED; }
static DWORD WINAPI
XInputGetCapabilitiesStub(DWORD index, DWORD flags, XINPUT_CAPABILITIES* state){ return ERROR_DEVICE_NOT_CONNECTED; }

static void
LoadXInput(void)
{
	static const char* const dll_names[] = { "xinput1_4.dll", "xinput9_1_0.dll", "xinput1_3.dll" };
	
	XInputGetState        = XInputGetStateStub;
	XInputSetState        = XInputSetStateStub;
	XInputGetCapabilities = XInputGetCapabilitiesStub;
	
	for (int32 i = 0; i < ArrayLength(dll_names); ++i)
	{
		HMODULE library = Win32_LoadLibrary(dll_names[i]);
		
		if (library)
		{
			XInputGetState        = (void*)GetProcAddress(library, "XInputGetState");
			XInputSetState        = (void*)GetProcAddress(library, "XInputSetState");
			XInputGetCapabilities = (void*)GetProcAddress(library, "XInputGetCapabilities");
			break;
		}
	}
}

static const char*
GetXInputSubTypeString(uint8 subtype)
{
	switch (subtype)
	{
		default:
		case XINPUT_DEVSUBTYPE_UNKNOWN:          return "Unknown";
		case XINPUT_DEVSUBTYPE_GAMEPAD:          return "Gamepad";
		case XINPUT_DEVSUBTYPE_WHEEL:            return "Racing Wheel";
		case XINPUT_DEVSUBTYPE_ARCADE_STICK:     return "Arcade Stick";
		case XINPUT_DEVSUBTYPE_FLIGHT_STICK:     return "Flight Stick";
		case XINPUT_DEVSUBTYPE_DANCE_PAD:        return "Dance Pad";
		case XINPUT_DEVSUBTYPE_GUITAR:           return "Guitar";
		case XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE: return "Guitar Alternate";
		case XINPUT_DEVSUBTYPE_GUITAR_BASS:      return "Guitar Bass";
		case XINPUT_DEVSUBTYPE_DRUM_KIT:         return "Drum";
		case XINPUT_DEVSUBTYPE_ARCADE_PAD:       return "Arcade Pad";
	}
}

static void
LoadDirectInput(void)
{
	static const char* const dll_names[] = { "dinput8.dll", /* "dinput.dll" */ };
	
	bool loaded = false;
	for (int32 i = 0; i < ArrayLength(dll_names); ++i)
	{
		HMODULE library = Win32_LoadLibrary(dll_names[i]);
		
		if (library)
		{
			DirectInput8Create = (void*)GetProcAddress(library, "DirectInput8Create");
			
			loaded = true;
			break;
		}
	}
	
	if (loaded)
	{
		HRESULT result = DirectInput8Create(g_win32.instance, DIRECTINPUT_VERSION, &IID_IDirectInput8W, (void**)&global_dinput, NULL);
		
		global_dinput_enabled = (result == DI_OK);
	}
}

static void
TranslateController(OS_GamepadState* out, const GamepadMappings* map,
	const bool8 buttons[32], const float32 axes[16], const int32 povs[8])
{
	Trace();
	
	//- Update Buttons
	for (int32 i = 0; i < ArrayLength(out->buttons); ++i)
		out->buttons[i].changes = 0;
	
	//- Translate Buttons
	for (int32 i = 0; i < ArrayLength(map->buttons); ++i)
	{
		if (!map->buttons[i])
			continue;
		
		OS_GamepadButton as_button = -1;
		
		uint8 lower = map->buttons[i] & 255;
		//uint8 higher = map->buttons[i] >> 8;
		
		switch (lower)
		{
			case GamepadObject_Button_A: as_button = OS_GamepadButton_A; break;
			case GamepadObject_Button_B: as_button = OS_GamepadButton_B; break;
			case GamepadObject_Button_X: as_button = OS_GamepadButton_X; break;
			case GamepadObject_Button_Y: as_button = OS_GamepadButton_Y; break;
			case GamepadObject_Button_Left: as_button = OS_GamepadButton_Left; break;
			case GamepadObject_Button_Right: as_button = OS_GamepadButton_Right; break;
			case GamepadObject_Button_Up: as_button = OS_GamepadButton_Up; break;
			case GamepadObject_Button_Down: as_button = OS_GamepadButton_Down; break;
			case GamepadObject_Button_LeftShoulder: as_button = OS_GamepadButton_LB; break;
			case GamepadObject_Button_RightShoulder: as_button = OS_GamepadButton_RB; break;
			case GamepadObject_Button_LeftStick: as_button = OS_GamepadButton_LS; break;
			case GamepadObject_Button_RightStick: as_button = OS_GamepadButton_RS; break;
			case GamepadObject_Button_Start: as_button = OS_GamepadButton_Start; break;
			case GamepadObject_Button_Back: as_button = OS_GamepadButton_Back; break;
			
			case GamepadObject_Pressure_LeftTrigger: out->lt = (float32)(buttons[i] != 0); break;
			case GamepadObject_Pressure_RightTrigger: out->rt = (float32)(buttons[i] != 0); break;
			
			// NOTE(ljre): Every other is invalid
			default: break;
		}
		
		if (as_button != -1)
		{
			bool is_down = buttons[i];
			
			out->buttons[as_button].changes = (out->buttons[as_button].is_down != is_down);
			out->buttons[as_button].is_down = is_down;
		}
	}
	
	//- Translate Axes
	for (int32 i = 0; i < ArrayLength(map->axes); ++i)
	{
		if (!map->axes[i])
			continue;
		
		uint8 lower = map->axes[i] & 255;
		uint8 higher = map->axes[i] >> 8;
		float32 value = axes[i];
		
		if (higher & (1 << 7))
			value = -value;
		
		// Input Limit
		if (higher & (1 << 6))
			value = value * 0.5f + 1.0f;
		else if (higher & (1 << 5))
			value = value * 2.0f - 1.0f;
		
		value = glm_clamp(value, -1.0f, 1.0f);
		
		// Output Limit
		if (higher & (1 << 4))
			value = value * 2.0f - 1.0f;
		else if (higher & (1 << 3))
			value = value * 0.5f + 1.0f;
		
		value = glm_clamp(value, -1.0f, 1.0f);
		
		switch (lower)
		{
			case GamepadObject_Axis_LeftX: out->left[0] = value; break;
			case GamepadObject_Axis_LeftY: out->left[1] = value; break;
			case GamepadObject_Axis_RightX: out->right[0] = value; break;
			case GamepadObject_Axis_RightY: out->right[1] = value; break;
			
			case GamepadObject_Pressure_LeftTrigger: out->lt = value; break;
			case GamepadObject_Pressure_RightTrigger: out->rt = value; break;
			
			// NOTE(ljre): Every other is invalid
			default: break;
		}
	}
	
	//- Translate Povs
	for (int32 i = 0; i < ArrayLength(map->povs); ++i)
	{
		if (!map->povs[i][0])
			continue;
		
		OS_GamepadButton as_button;
		int32 pov = povs[i];
		uint8 lower, higher;
		uint16 object;
		
		(void)higher;
		
		if (pov == -1)
			continue;
		
		as_button = -1;
		object = map->povs[i][pov >> 1];
		lower = object & 255;
		higher = object >> 8;
		switch (lower)
		{
			case GamepadObject_Button_A: as_button = OS_GamepadButton_A; break;
			case GamepadObject_Button_B: as_button = OS_GamepadButton_B; break;
			case GamepadObject_Button_X: as_button = OS_GamepadButton_X; break;
			case GamepadObject_Button_Y: as_button = OS_GamepadButton_Y; break;
			case GamepadObject_Button_Left: as_button = OS_GamepadButton_Left; break;
			case GamepadObject_Button_Right: as_button = OS_GamepadButton_Right; break;
			case GamepadObject_Button_Up: as_button = OS_GamepadButton_Up; break;
			case GamepadObject_Button_Down: as_button = OS_GamepadButton_Down; break;
			case GamepadObject_Button_LeftShoulder: as_button = OS_GamepadButton_LB; break;
			case GamepadObject_Button_RightShoulder: as_button = OS_GamepadButton_RB; break;
			case GamepadObject_Button_LeftStick: as_button = OS_GamepadButton_LS; break;
			case GamepadObject_Button_RightStick: as_button = OS_GamepadButton_RS; break;
			case GamepadObject_Button_Start: as_button = OS_GamepadButton_Start; break;
			case GamepadObject_Button_Back: as_button = OS_GamepadButton_Back; break;
			
			default: break;
		}
		
		if (as_button != -1)
		{
			bool is_down = buttons[i];
			
			out->buttons[as_button].changes = (out->buttons[as_button].is_down != is_down);
			out->buttons[as_button].is_down = is_down;
		}
		
		// NOTE(ljre): if it's not pointing to a diagonal, ignore next step
		if ((pov & 1) == 0)
			continue;
		
		as_button = -1;
		object = map->povs[i][(pov+1 & 7) >> 1];
		lower = object & 255;
		higher = object >> 8;
		switch (lower)
		{
			case GamepadObject_Button_A: as_button = OS_GamepadButton_A; break;
			case GamepadObject_Button_B: as_button = OS_GamepadButton_B; break;
			case GamepadObject_Button_X: as_button = OS_GamepadButton_X; break;
			case GamepadObject_Button_Y: as_button = OS_GamepadButton_Y; break;
			case GamepadObject_Button_Left: as_button = OS_GamepadButton_Left; break;
			case GamepadObject_Button_Right: as_button = OS_GamepadButton_Right; break;
			case GamepadObject_Button_Up: as_button = OS_GamepadButton_Up; break;
			case GamepadObject_Button_Down: as_button = OS_GamepadButton_Down; break;
			case GamepadObject_Button_LeftShoulder: as_button = OS_GamepadButton_LB; break;
			case GamepadObject_Button_RightShoulder: as_button = OS_GamepadButton_RB; break;
			case GamepadObject_Button_LeftStick: as_button = OS_GamepadButton_LS; break;
			case GamepadObject_Button_RightStick: as_button = OS_GamepadButton_RS; break;
			case GamepadObject_Button_Start: as_button = OS_GamepadButton_Start; break;
			case GamepadObject_Button_Back: as_button = OS_GamepadButton_Back; break;
			
			default: break;
		}
		
		if (as_button != -1)
		{
			bool is_down = buttons[i];
			
			out->buttons[as_button].changes = (out->buttons[as_button].is_down != is_down);
			out->buttons[as_button].is_down = is_down;
		}
	}
	
	//- The end of your pain
	NormalizeAxis(out->left);
	NormalizeAxis(out->right);
}

static void
UpdateConnectedGamepad(Win32_Gamepad* gamepad)
{
	Trace();
	
	int32 index = (int32)(gamepad - global_gamepads) / (int32)(sizeof(*gamepad));
	
	switch (gamepad->api)
	{
		//~ DirectInput
		case Win32_GamepadAPI_DirectInput:
		{
			DIJOYSTATE2 state;
			IDirectInputDevice8_Poll(gamepad->dinput.device);
			
			HRESULT result = IDirectInputDevice8_GetDeviceState(gamepad->dinput.device, sizeof(state), &state);
			if (result == DIERR_NOTACQUIRED || result == DIERR_INPUTLOST)
			{
				IDirectInputDevice8_Acquire(gamepad->dinput.device);
				IDirectInputDevice8_Poll(gamepad->dinput.device);
				
				result = IDirectInputDevice8_GetDeviceState(gamepad->dinput.device, sizeof(state), &state);
			}
			
			if (result == DI_OK)
			{
				bool8 buttons[32];
				float32 axes[16];
				int32 povs[8];
				
				int32 axis_count = gamepad->dinput.axis_count;
				if (axis_count > ArrayLength(axes))
					axis_count = ArrayLength(axes);
				
				int32 button_count = gamepad->dinput.button_count;
				if (button_count > ArrayLength(buttons))
					button_count = ArrayLength(buttons);
				
				int32 pov_count = gamepad->dinput.pov_count;
				if (pov_count > ArrayLength(povs))
					pov_count = ArrayLength(povs);
				
				for (int32 i = 0; i < axis_count; ++i)
				{
					LONG raw = *(LONG*)((uint8*)&state + gamepad->dinput.axes[i]);
					axes[i] = ((float32)raw + 0.5f) / 32767.5f;
				}
				
				for (int32 i = 0; i < button_count; ++i)
				{
					LONG raw = *(LONG*)((uint8*)&state + gamepad->dinput.buttons[i]);
					buttons[i] = !!(raw & 128);
				}
				
				for (int32 i = 0; i < pov_count; ++i)
				{
					LONG raw = *(LONG*)((uint8*)&state + gamepad->dinput.povs[i]);
					if (raw == -1)
					{
						povs[i] = -1;
						continue;
					}
					
					// '(x%n + n) % n' is the same as 'x & (n-1)' when 'n' is a power of 2
					// negative values are going to wrap around
					raw = (raw / 4500) & 7;
					
					// Value of 'raw' is, assuming order Right, Up, Left, Down:
					// 0 = Right
					// 1 = Right & Up
					// 2 = Up
					// 3 = Up & Left
					// 4 = Left
					// 5 = Left & Down
					// 6 = Down
					// 7 = Down & Right
					
					povs[i] = (int32)raw;
				}
				
				TranslateController(&gamepad->data, gamepad->dinput.map, buttons, axes, povs);
			}
			else
			{
				IDirectInputDevice8_Unacquire(gamepad->dinput.device);
				IDirectInputDevice8_Release(gamepad->dinput.device);
				
				gamepad->connected = false;
				gamepad->api = Win32_GamepadAPI_None;
				ReleaseGamepadIndex(index);
			}
		} break;
		
		//~ XInput
		case Win32_GamepadAPI_XInput:
		{
			XINPUT_STATE state;
			DWORD result = XInputGetState(gamepad->xinput.index, &state);
			
			if (result == ERROR_SUCCESS)
			{
				//- Sticks
				gamepad->data.left[0] = ((float32)state.Gamepad.sThumbLX + 0.5f) /  32767.5f;
				gamepad->data.left[1] = ((float32)state.Gamepad.sThumbLY + 0.5f) / -32767.5f;
				NormalizeAxis(gamepad->data.left);
				
				gamepad->data.right[0] = ((float32)state.Gamepad.sThumbRX + 0.5f) /  32767.5f;
				gamepad->data.right[1] = ((float32)state.Gamepad.sThumbRY + 0.5f) / -32767.5f;
				NormalizeAxis(gamepad->data.right);
				
				//- LR & RB
				gamepad->data.lt = (float32)state.Gamepad.bLeftTrigger / 255.0f;
				gamepad->data.rt = (float32)state.Gamepad.bRightTrigger / 255.0f;
				
				//- Buttons
				struct
				{
					OS_GamepadButton gpad;
					int32 xinput;
				}
				static const table[] = {
					{ OS_GamepadButton_Left, XINPUT_GAMEPAD_DPAD_LEFT },
					{ OS_GamepadButton_Right, XINPUT_GAMEPAD_DPAD_RIGHT },
					{ OS_GamepadButton_Up, XINPUT_GAMEPAD_DPAD_UP },
					{ OS_GamepadButton_Down, XINPUT_GAMEPAD_DPAD_DOWN },
					{ OS_GamepadButton_Start, XINPUT_GAMEPAD_START },
					{ OS_GamepadButton_Back, XINPUT_GAMEPAD_BACK },
					{ OS_GamepadButton_LS, XINPUT_GAMEPAD_LEFT_THUMB },
					{ OS_GamepadButton_RS, XINPUT_GAMEPAD_RIGHT_THUMB },
					{ OS_GamepadButton_LB, XINPUT_GAMEPAD_LEFT_SHOULDER },
					{ OS_GamepadButton_RB, XINPUT_GAMEPAD_RIGHT_SHOULDER },
					{ OS_GamepadButton_A, XINPUT_GAMEPAD_A },
					{ OS_GamepadButton_B, XINPUT_GAMEPAD_B },
					{ OS_GamepadButton_X, XINPUT_GAMEPAD_X },
					{ OS_GamepadButton_Y, XINPUT_GAMEPAD_Y },
				};
				
				for (int32 i = 0; i < ArrayLength(table); ++i)
				{
					bool is_down = state.Gamepad.wButtons & table[i].xinput;
					OS_ButtonState* button = &gamepad->data.buttons[table[i].gpad];
					
					button->changes = button->is_down != is_down;
					button->is_down = is_down;
				}
			}
			else if (result == ERROR_DEVICE_NOT_CONNECTED)
			{
				gamepad->api = Win32_GamepadAPI_None;
				gamepad->connected = false;
				ReleaseGamepadIndex(index);
			}
		} break;
		
		case Win32_GamepadAPI_None:
		{
			// NOTE(ljre): This should be unreachable... just to be safe
			gamepad->connected = false;
			ReleaseGamepadIndex(index);
		} break;
	}
}

static BOOL CALLBACK
DirectInputEnumObjectsCallback(const DIDEVICEOBJECTINSTANCEW* object, void* userdata)
{
	Win32_Gamepad* gamepad = userdata;
	const GUID* object_guid = &object->guidType;
	
	if (DIDFT_GETTYPE(object->dwType) & DIDFT_AXIS)
	{
		int32 axis_offset;
		DIPROPRANGE range_property = {
			.diph = {
				.dwSize = sizeof(range_property),
				.dwHeaderSize = sizeof(range_property.diph),
				.dwObj = object->dwType,
				.dwHow = DIPH_BYID,
			},
			.lMin = INT16_MIN,
			.lMax = INT16_MAX,
		};
		
		if (AreGUIDsEqual(object_guid, &GUID_Slider))
		{
			axis_offset = (int32)DIJOFS_SLIDER(gamepad->dinput.slider_count);
			axis_offset |= 0x40000000;
		}
		else if (AreGUIDsEqual(object_guid, &GUID_XAxis )) axis_offset = (int32)DIJOFS_X;
		else if (AreGUIDsEqual(object_guid, &GUID_YAxis )) axis_offset = (int32)DIJOFS_Y;
		else if (AreGUIDsEqual(object_guid, &GUID_ZAxis )) axis_offset = (int32)DIJOFS_Z;
		else if (AreGUIDsEqual(object_guid, &GUID_RxAxis)) axis_offset = (int32)DIJOFS_RX;
		else if (AreGUIDsEqual(object_guid, &GUID_RyAxis)) axis_offset = (int32)DIJOFS_RY;
		else if (AreGUIDsEqual(object_guid, &GUID_RzAxis)) axis_offset = (int32)DIJOFS_RZ;
		else return DIENUM_CONTINUE;
		
		if (DI_OK != IDirectInputDevice8_SetProperty(gamepad->dinput.device, DIPROP_RANGE, &range_property.diph))
			return DIENUM_CONTINUE;
		
		if (axis_offset & 0x40000000)
			++gamepad->dinput.slider_count;
		
		gamepad->dinput.axes[gamepad->dinput.axis_count++] = axis_offset;
	}
	else if (DIDFT_GETTYPE(object->dwType) & DIDFT_BUTTON)
	{
		int32 button_offset = (int32)DIJOFS_BUTTON(gamepad->dinput.button_count);
		gamepad->dinput.buttons[gamepad->dinput.button_count++] = button_offset;
	}
	else if (DIDFT_GETTYPE(object->dwType) & DIDFT_POV)
	{
		int32 pov_offset = (int32)DIJOFS_POV(gamepad->dinput.pov_count);
		gamepad->dinput.povs[gamepad->dinput.pov_count++] = pov_offset;
	}
	
	return DIENUM_CONTINUE;
}

static void
SortGamepadObjectInt(int32* array, intsize count)
{
	for (intsize i = 0; i < count; ++i)
	{
		for (intsize j = 0; j < count-1-i; ++j)
		{
			if (array[j] > array[j+1])
			{
				int32 tmp = array[j];
				array[j] = array[j+1];
				array[j+1] = tmp;
			}
		}
	}
}

static void
SortGamepadObjects(Win32_Gamepad* gamepad)
{
	SortGamepadObjectInt(gamepad->dinput.buttons, gamepad->dinput.button_count);
	SortGamepadObjectInt(gamepad->dinput.axes, gamepad->dinput.axis_count);
	SortGamepadObjectInt(gamepad->dinput.povs, gamepad->dinput.pov_count);
	
	// Cleanup
	for (int32 i = 0; i < gamepad->dinput.axis_count; ++i)
		gamepad->dinput.axes[i] &= ~0x40000000;
}

static BOOL CALLBACK
DirectInputEnumDevicesCallback(const DIDEVICEINSTANCEW* instance, void* userdata)
{
	Trace();
	
	if (global_gamepad_free_count <= 0)
		return DIENUM_STOP;
	
	const GUID* guid = &instance->guidInstance;
	if (XInputGetState != XInputGetStateStub && IsXInputDevice(&instance->guidProduct))
		return DIENUM_CONTINUE;
	
	// Check if device is already added
	for (int32 i = 0; i < ArrayLength(global_gamepads); ++i)
	{
		if (global_gamepads[i].connected &&
			global_gamepads[i].api == Win32_GamepadAPI_DirectInput &&
			AreGUIDsEqual(guid, &global_gamepads[i].dinput.guid))
		{
			return DIENUM_CONTINUE;
		}
	}
	
	// It's not! Add it!
	int32 index = GenGamepadIndex();
	Win32_Gamepad* gamepad = &global_gamepads[index];
	gamepad->dinput.axis_count = 0;
	gamepad->dinput.slider_count = 0;
	gamepad->dinput.button_count = 0;
	gamepad->dinput.pov_count = 0;
	
	DIDEVCAPS capabilities = {
		.dwSize = sizeof(capabilities),
	};
	DIPROPDWORD word_property = {
		.diph = {
			.dwSize = sizeof(word_property),
			.dwHeaderSize = sizeof(word_property.diph),
			.dwObj = 0,
			.dwHow = DIPH_DEVICE,
		},
		.dwData = DIPROPAXISMODE_ABS,
	};
	
	if (DI_OK != IDirectInput8_CreateDevice(global_dinput, guid, &gamepad->dinput.device, NULL))
		goto _failure;
	
	if (DI_OK != IDirectInputDevice8_SetCooperativeLevel(gamepad->dinput.device, g_win32.window, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))
		goto _failure;
	
	if (DI_OK != IDirectInputDevice8_SetDataFormat(gamepad->dinput.device, &c_dfDIJoystick2))
		goto _failure;
	
	if (DI_OK != IDirectInputDevice8_GetCapabilities(gamepad->dinput.device, &capabilities))
		goto _failure;
	
	if (DI_OK != IDirectInputDevice8_SetProperty(gamepad->dinput.device, DIPROP_AXISMODE, &word_property.diph))
		goto _failure;
	
	if (DI_OK != IDirectInputDevice8_EnumObjects(gamepad->dinput.device, DirectInputEnumObjectsCallback, gamepad, DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV))
		goto _failure;
	
	if (DI_OK != IDirectInputDevice8_Acquire(gamepad->dinput.device))
		goto _failure;
	
	// Success
	gamepad->connected = true;
	gamepad->api = Win32_GamepadAPI_DirectInput;
	gamepad->dinput.guid = *guid;
	gamepad->dinput.map = &global_gamepadmap_default;
	
	SortGamepadObjects(gamepad);
	
	int32 index_of_map_being_used = -1;
	
	// NOTE(ljre): Look for a mapping for this gamepad
	char guid_str[64];
	
	if (ConvertGuidToSDLGuid(instance, guid_str, sizeof(guid_str)))
	{
		for (int32 i = 0; i < ArrayLength(global_gamepadmap_database); ++i)
		{
			const GamepadMappings* map = &global_gamepadmap_database[i];
			
			if (Mem_Compare(guid_str, map->guid, ArrayLength(map->guid)) == 0)
			{
				gamepad->dinput.map = map;
				index_of_map_being_used = i;
				break;
			}
		}
	}
	
	(void)index_of_map_being_used;
#ifdef DEBUG
	OS_DebugLog("[info-win32] Device Connected:\n");
	OS_DebugLog("\tIndex: %i\n", index);
	OS_DebugLog("\tDriver: DirectInput\n");
	//OS_DebugLog("\tName: %w\n", instance->tszInstanceName);
	//OS_DebugLog("\tProduct Name: %w\n", instance->tszProductName);
	OS_DebugLog("\tGUID Instance: %08x-%04x-%04x-%04x-%04x-%08x\n",
		guid->Data1, guid->Data2, guid->Data3,
		*(uint16*)guid->Data4, ((uint16*)guid->Data4)[1], *(uint32*)guid->Data4);
	OS_DebugLog("\tGUID Product: %08x-%04x-%04x-%04x-%04x-%08x\n",
		instance->guidProduct.Data1, instance->guidProduct.Data2, instance->guidProduct.Data3,
		*(uint16*)instance->guidProduct.Data4, ((uint16*)instance->guidProduct.Data4)[1], *(uint32*)instance->guidProduct.Data4);
	
	if (index_of_map_being_used == -1)
		OS_DebugLog("\tMappings: Default\n");
	else
		OS_DebugLog("\tMappings: Index %i\n", index_of_map_being_used);
	
	OS_DebugLog("\tAxes: { ");
	for (int32 i = 0; i < gamepad->dinput.axis_count; ++i)
		OS_DebugLog("%02x, ", (uint32)gamepad->dinput.axes[i]);
	OS_DebugLog("}\n");
	
	OS_DebugLog("\tButtons: { ");
	for (int32 i = 0; i < gamepad->dinput.button_count; ++i)
		OS_DebugLog("%02x, ", (uint32)gamepad->dinput.buttons[i]);
	OS_DebugLog("}\n");
	
	OS_DebugLog("\tPOVs: { ");
	for (int32 i = 0; i < gamepad->dinput.pov_count; ++i)
		OS_DebugLog("%02x, ", (uint32)gamepad->dinput.povs[i]);
	OS_DebugLog("}\n");
#endif
	
	return DIENUM_CONTINUE;
	
	//-
	_failure:
	{
		gamepad->connected = false;
		gamepad->api = Win32_GamepadAPI_None;
		IDirectInputDevice8_Release(gamepad->dinput.device);
		ReleaseGamepadIndex(index);
		
		return DIENUM_CONTINUE;
	}
}

//~ Internal API
static void
Win32_CheckForGamepads(void)
{
	Trace();
	
	// NOTE(ljre): Don't check for more devices if there isn't more space
	if (global_gamepad_free_count <= 0)
		return;
	
	// Direct Input
	if (global_dinput)
		IDirectInput8_EnumDevices(global_dinput, DI8DEVCLASS_GAMECTRL, DirectInputEnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
	
	// XInput
	for (DWORD i = 0; i < 4 && global_gamepad_free_count > 0; ++i)
	{
		bool already_exists = false;
		
		for (int32 index = 0; index < ArrayLength(global_gamepads); ++index)
		{
			if (global_gamepads[index].connected &&
				global_gamepads[index].api == Win32_GamepadAPI_XInput &&
				global_gamepads[index].xinput.index == i)
			{
				already_exists = true;
			}
		}
		
		XINPUT_CAPABILITIES cap;
		if (!already_exists && XInputGetCapabilities(i, 0, &cap) == ERROR_SUCCESS)
		{
			int32 index = GenGamepadIndex();
			Win32_Gamepad* gamepad = &global_gamepads[index];
			
			gamepad->connected = true;
			gamepad->api = Win32_GamepadAPI_XInput;
			gamepad->xinput.index = i;
			
			OS_DebugLog("[info-win32] Device Connected:\n");
			OS_DebugLog("\tIndex: %i\n", index);
			OS_DebugLog("\tDriver: XInput\n");
			OS_DebugLog("\tSubtype: %s Controller\n", GetXInputSubTypeString(cap.SubType));
		}
	}
}

static bool
Win32_InitInput(void)
{
	Trace();
	
	// NOTE(ljre): Fill 'free spaces stack'
	global_gamepad_free_count = ArrayLength(global_gamepads);
	for (int32 i = 0; i < ArrayLength(global_gamepads); ++i)
		global_gamepad_free[(int32)ArrayLength(global_gamepads) - i - 1] = i;
	
	LoadXInput();
	LoadDirectInput();
	
	Win32_CheckForGamepads();
	
	return true;
}

static void
Win32_UpdateKeyboardKey(OS_State* os_state, uint32 vkcode, bool is_down)
{
	OS_KeyboardKey key = global_keyboard_key_table[vkcode];
	
	if (key)
	{
		OS_ButtonState* btn = &os_state->keyboard.buttons[key];
		
		if (btn->is_down != is_down)
		{
			btn->changes += 1;
			btn->is_down = is_down;
		}
		
		if (is_down)
		{
			if (os_state->keyboard.buffered_count < ArrayLength(os_state->keyboard.buffered))
				os_state->keyboard.buffered[os_state->keyboard.buffered_count++] = (uint8)key;
			else
				OS_DebugLog("[warning-win32] Lost buffered input: %u\n");
		}
	}
}

static void
Win32_UpdateMouseButton(OS_State* os_state, OS_MouseButton button, bool is_down)
{
	os_state->mouse.buttons[button].changes += 1;
	os_state->mouse.buttons[button].is_down = is_down;
}

static inline void
Win32_UpdateInputEarly(OS_State* os_state)
{
	// NOTE(ljre): Update Keyboard
	for (int32 i = 0; i < ArrayLength(os_state->keyboard.buttons); ++i)
		os_state->keyboard.buttons[i].changes = 0;
	
	os_state->keyboard.buffered_count = 0;
	
	// NOTE(ljre): Update Mouse
	for (int32 i = 0; i < ArrayLength(os_state->mouse.buttons); ++i)
		os_state->mouse.buttons[i].changes = 0;
	
	os_state->mouse.scroll = 0;
	
	// NOTE(ljre): Update text
	os_state->text_codepoints_count = 0;
	Mem_Zero(os_state->text_codepoints, sizeof(os_state->text_codepoints));
}

static inline void
Win32_UpdateInputLate(OS_State* os_state)
{
	// NOTE(ljre): Get mouse cursor
	{
		os_state->mouse.old_pos[0] = os_state->mouse.pos[0];
		os_state->mouse.old_pos[1] = os_state->mouse.pos[1];
		
		POINT mouse;
		GetCursorPos(&mouse);
		ScreenToClient(g_win32.window, &mouse);
		
		os_state->mouse.pos[0] = glm_clamp((float32)mouse.x, 0.0f, (float32)os_state->window.width);
		os_state->mouse.pos[1] = glm_clamp((float32)mouse.y, 0.0f, (float32)os_state->window.height);
		
		static bool prev_update_mouse_pos = false;
		bool update_mouse_pos = (GetForegroundWindow() == g_win32.window);
		
		if (g_win32.lock_cursor && update_mouse_pos)
		{
			mouse.x = os_state->window.width/2;
			mouse.y = os_state->window.height/2;
			
			ClientToScreen(g_win32.window, &mouse);
			SetCursorPos(mouse.x, mouse.y);
			
			os_state->mouse.old_pos[0] = (float32)(os_state->window.width/2);
			os_state->mouse.old_pos[1] = (float32)(os_state->window.height/2);
			
			if (!prev_update_mouse_pos)
			{
				os_state->mouse.pos[0] = os_state->mouse.old_pos[0];
				os_state->mouse.pos[1] = os_state->mouse.old_pos[1];
			}
		}
		
		prev_update_mouse_pos = update_mouse_pos;
	}
	
	// NOTE(ljre): Update Gamepads
	os_state->connected_gamepads = 0;
	
	for (int32 i = 0; i < ArrayLength(global_gamepads); ++i)
	{
		Win32_Gamepad* gamepad = &global_gamepads[i];
		
		if (gamepad->connected)
		{
			os_state->connected_gamepads |= 1 << i;
			
			UpdateConnectedGamepad(gamepad);
			os_state->gamepads[i] = gamepad->data;
		}
		else
			Mem_Zero(&os_state->gamepads[i], sizeof(OS_GamepadState));
	}
}
