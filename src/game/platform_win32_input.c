#include "internal_gamepad_map.h"

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
	bool32 connected;
	Engine_GamepadState data;
	
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
internal Win32_Gamepad   global_gamepads[Engine_MAX_GAMEPAD_COUNT];
internal int32           global_gamepad_free[Engine_MAX_GAMEPAD_COUNT];
internal int32           global_gamepad_free_count;
internal IDirectInput8W* global_dinput;
internal bool32          global_dinput_enabled;

internal ProcXInputGetState*        XInputGetState;
internal ProcXInputSetState*        XInputSetState;
internal ProcXInputGetCapabilities* XInputGetCapabilities;
internal ProcDirectInput8Create*    DirectInput8Create;

internal const Engine_KeyboardKey global_keyboard_key_table[] = {
	[0] = Engine_KeyboardKey_Any,
	
	[VK_ESCAPE] = Engine_KeyboardKey_Escape,
	[VK_CONTROL] = Engine_KeyboardKey_Control,
	[VK_SHIFT] = Engine_KeyboardKey_Shift,
	[VK_MENU] = Engine_KeyboardKey_Alt,
	[VK_TAB] = Engine_KeyboardKey_Tab,
	[VK_RETURN] = Engine_KeyboardKey_Enter,
	[VK_BACK] = Engine_KeyboardKey_Backspace,
	[VK_UP] = Engine_KeyboardKey_Up,
	[VK_DOWN] = Engine_KeyboardKey_Down,
	[VK_LEFT] = Engine_KeyboardKey_Left,
	[VK_RIGHT] = Engine_KeyboardKey_Right,
	[VK_LCONTROL] = Engine_KeyboardKey_LeftControl,
	[VK_RCONTROL] = Engine_KeyboardKey_RightControl,
	[VK_LSHIFT] = Engine_KeyboardKey_LeftShift,
	[VK_RSHIFT] = Engine_KeyboardKey_RightShift,
	[VK_PRIOR] = Engine_KeyboardKey_PageUp,
	[VK_NEXT] = Engine_KeyboardKey_PageDown,
	[VK_END] = Engine_KeyboardKey_End,
	[VK_HOME] = Engine_KeyboardKey_Home,
	
	[VK_SPACE] = Engine_KeyboardKey_Space,
	
	['0'] = Engine_KeyboardKey_0,
	Engine_KeyboardKey_1, Engine_KeyboardKey_2, Engine_KeyboardKey_3, Engine_KeyboardKey_4,
	Engine_KeyboardKey_5, Engine_KeyboardKey_6, Engine_KeyboardKey_7, Engine_KeyboardKey_8,
	Engine_KeyboardKey_9,
	
	['A'] = Engine_KeyboardKey_A,
	Engine_KeyboardKey_B, Engine_KeyboardKey_C, Engine_KeyboardKey_D, Engine_KeyboardKey_E,
	Engine_KeyboardKey_F, Engine_KeyboardKey_G, Engine_KeyboardKey_H, Engine_KeyboardKey_I,
	Engine_KeyboardKey_J, Engine_KeyboardKey_K, Engine_KeyboardKey_L, Engine_KeyboardKey_M,
	Engine_KeyboardKey_N, Engine_KeyboardKey_O, Engine_KeyboardKey_P, Engine_KeyboardKey_Q,
	Engine_KeyboardKey_R, Engine_KeyboardKey_S, Engine_KeyboardKey_T, Engine_KeyboardKey_U,
	Engine_KeyboardKey_V, Engine_KeyboardKey_W, Engine_KeyboardKey_X, Engine_KeyboardKey_Y,
	Engine_KeyboardKey_Z,
	
	[VK_F1] = Engine_KeyboardKey_F1,
	Engine_KeyboardKey_F2, Engine_KeyboardKey_F3,  Engine_KeyboardKey_F4,  Engine_KeyboardKey_F4,
	Engine_KeyboardKey_F5, Engine_KeyboardKey_F6,  Engine_KeyboardKey_F7,  Engine_KeyboardKey_F8,
	Engine_KeyboardKey_F9, Engine_KeyboardKey_F10, Engine_KeyboardKey_F11, Engine_KeyboardKey_F12,
	Engine_KeyboardKey_F13,
	
	[VK_NUMPAD0] = Engine_KeyboardKey_Numpad0,
	Engine_KeyboardKey_Numpad1, Engine_KeyboardKey_Numpad2, Engine_KeyboardKey_Numpad3, Engine_KeyboardKey_Numpad4,
	Engine_KeyboardKey_Numpad5, Engine_KeyboardKey_Numpad6, Engine_KeyboardKey_Numpad7, Engine_KeyboardKey_Numpad8,
	Engine_KeyboardKey_Numpad9,
};

//~ Functions
#include "guid_utils.h"

internal inline bool32
AreGUIDsEqual(const GUID* a, const GUID* b)
{
	return MemCmp(a, b, sizeof (GUID)) == 0;
}

internal inline int32
GenGamepadIndex(void)
{
	return global_gamepad_free[--global_gamepad_free_count];
}

internal inline void
ReleaseGamepadIndex(int32 index)
{
	global_gamepad_free[global_gamepad_free_count++] = index;
}

internal inline void
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

internal DWORD WINAPI
XInputGetStateStub(DWORD index, XINPUT_STATE* state) { return ERROR_DEVICE_NOT_CONNECTED; }
internal DWORD WINAPI
XInputSetStateStub(DWORD index, XINPUT_VIBRATION* state) { return ERROR_DEVICE_NOT_CONNECTED; }
internal DWORD WINAPI
XInputGetCapabilitiesStub(DWORD index, DWORD flags, XINPUT_CAPABILITIES* state){ return ERROR_DEVICE_NOT_CONNECTED; }

internal void
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

internal const char*
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

internal void
LoadDirectInput(void)
{
	static const char* const dll_names[] = { "dinput8.dll", /* "dinput.dll" */ };
	
	bool32 loaded = false;
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
		HRESULT result = DirectInput8Create(global_instance, DIRECTINPUT_VERSION, &IID_IDirectInput8W, (void**)&global_dinput, NULL);
		
		global_dinput_enabled = (result == DI_OK);
	}
}

internal void
TranslateController(Engine_GamepadState* out, const GamepadMappings* map,
					const bool8 buttons[32], const float32 axes[16], const int32 povs[8])
{
	Trace();
	
	//- Update Buttons
	for (int32 i = 0; i < ArrayLength(out->buttons); ++i)
	{
		out->buttons[i].changes = 0;
	}
	
	//- Translate Buttons
	for (int32 i = 0; i < ArrayLength(map->buttons); ++i)
	{
		if (!map->buttons[i])
			continue;
		
		Engine_GamepadButton as_button = -1;
		
		uint8 lower = map->buttons[i] & 255;
		//uint8 higher = map->buttons[i] >> 8;
		
		switch (lower)
		{
			case GamepadObject_Button_A: as_button = Engine_GamepadButton_A; break;
			case GamepadObject_Button_B: as_button = Engine_GamepadButton_B; break;
			case GamepadObject_Button_X: as_button = Engine_GamepadButton_X; break;
			case GamepadObject_Button_Y: as_button = Engine_GamepadButton_Y; break;
			case GamepadObject_Button_Left: as_button = Engine_GamepadButton_Left; break;
			case GamepadObject_Button_Right: as_button = Engine_GamepadButton_Right; break;
			case GamepadObject_Button_Up: as_button = Engine_GamepadButton_Up; break;
			case GamepadObject_Button_Down: as_button = Engine_GamepadButton_Down; break;
			case GamepadObject_Button_LeftShoulder: as_button = Engine_GamepadButton_LB; break;
			case GamepadObject_Button_RightShoulder: as_button = Engine_GamepadButton_RB; break;
			case GamepadObject_Button_LeftStick: as_button = Engine_GamepadButton_LS; break;
			case GamepadObject_Button_RightStick: as_button = Engine_GamepadButton_RS; break;
			case GamepadObject_Button_Start: as_button = Engine_GamepadButton_Start; break;
			case GamepadObject_Button_Back: as_button = Engine_GamepadButton_Back; break;
			
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
		
		Engine_GamepadButton as_button;
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
			case GamepadObject_Button_A: as_button = Engine_GamepadButton_A; break;
			case GamepadObject_Button_B: as_button = Engine_GamepadButton_B; break;
			case GamepadObject_Button_X: as_button = Engine_GamepadButton_X; break;
			case GamepadObject_Button_Y: as_button = Engine_GamepadButton_Y; break;
			case GamepadObject_Button_Left: as_button = Engine_GamepadButton_Left; break;
			case GamepadObject_Button_Right: as_button = Engine_GamepadButton_Right; break;
			case GamepadObject_Button_Up: as_button = Engine_GamepadButton_Up; break;
			case GamepadObject_Button_Down: as_button = Engine_GamepadButton_Down; break;
			case GamepadObject_Button_LeftShoulder: as_button = Engine_GamepadButton_LB; break;
			case GamepadObject_Button_RightShoulder: as_button = Engine_GamepadButton_RB; break;
			case GamepadObject_Button_LeftStick: as_button = Engine_GamepadButton_LS; break;
			case GamepadObject_Button_RightStick: as_button = Engine_GamepadButton_RS; break;
			case GamepadObject_Button_Start: as_button = Engine_GamepadButton_Start; break;
			case GamepadObject_Button_Back: as_button = Engine_GamepadButton_Back; break;
			
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
			case GamepadObject_Button_A: as_button = Engine_GamepadButton_A; break;
			case GamepadObject_Button_B: as_button = Engine_GamepadButton_B; break;
			case GamepadObject_Button_X: as_button = Engine_GamepadButton_X; break;
			case GamepadObject_Button_Y: as_button = Engine_GamepadButton_Y; break;
			case GamepadObject_Button_Left: as_button = Engine_GamepadButton_Left; break;
			case GamepadObject_Button_Right: as_button = Engine_GamepadButton_Right; break;
			case GamepadObject_Button_Up: as_button = Engine_GamepadButton_Up; break;
			case GamepadObject_Button_Down: as_button = Engine_GamepadButton_Down; break;
			case GamepadObject_Button_LeftShoulder: as_button = Engine_GamepadButton_LB; break;
			case GamepadObject_Button_RightShoulder: as_button = Engine_GamepadButton_RB; break;
			case GamepadObject_Button_LeftStick: as_button = Engine_GamepadButton_LS; break;
			case GamepadObject_Button_RightStick: as_button = Engine_GamepadButton_RS; break;
			case GamepadObject_Button_Start: as_button = Engine_GamepadButton_Start; break;
			case GamepadObject_Button_Back: as_button = Engine_GamepadButton_Back; break;
			
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

internal void
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
					Engine_GamepadButton gpad;
					int32 xinput;
				}
				const table[] = {
					{ Engine_GamepadButton_Left, XINPUT_GAMEPAD_DPAD_LEFT },
					{ Engine_GamepadButton_Right, XINPUT_GAMEPAD_DPAD_RIGHT },
					{ Engine_GamepadButton_Up, XINPUT_GAMEPAD_DPAD_UP },
					{ Engine_GamepadButton_Down, XINPUT_GAMEPAD_DPAD_DOWN },
					{ Engine_GamepadButton_Start, XINPUT_GAMEPAD_START },
					{ Engine_GamepadButton_Back, XINPUT_GAMEPAD_BACK },
					{ Engine_GamepadButton_LS, XINPUT_GAMEPAD_LEFT_THUMB },
					{ Engine_GamepadButton_RS, XINPUT_GAMEPAD_RIGHT_THUMB },
					{ Engine_GamepadButton_LB, XINPUT_GAMEPAD_LEFT_SHOULDER },
					{ Engine_GamepadButton_RB, XINPUT_GAMEPAD_RIGHT_SHOULDER },
					{ Engine_GamepadButton_A, XINPUT_GAMEPAD_A },
					{ Engine_GamepadButton_B, XINPUT_GAMEPAD_B },
					{ Engine_GamepadButton_X, XINPUT_GAMEPAD_X },
					{ Engine_GamepadButton_Y, XINPUT_GAMEPAD_Y },
				};
				
				for (int32 i = 0; i < ArrayLength(table); ++i)
				{
					bool is_down = state.Gamepad.wButtons & table[i].xinput;
					Engine_ButtonState* button = &gamepad->data.buttons[table[i].gpad];
					
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

internal BOOL CALLBACK
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

internal int32
SortGamepadCompare(const void* a, const void* b)
{
	int32 left  = *(const int32*)a;
	int32 right = *(const int32*)b;
	
	return left - right;
}

internal void
SortGamepadObjects(Win32_Gamepad* gamepad)
{
	qsort(gamepad->dinput.buttons, (uintsize)gamepad->dinput.button_count, sizeof(int32), SortGamepadCompare);
	qsort(gamepad->dinput.axes, (uintsize)gamepad->dinput.axis_count, sizeof(int32), SortGamepadCompare);
	qsort(gamepad->dinput.povs, (uintsize)gamepad->dinput.pov_count, sizeof(int32), SortGamepadCompare);
	
	// Cleanup
	for (int32 i = 0; i < gamepad->dinput.axis_count; ++i)
	{
		gamepad->dinput.axes[i] &= ~0x40000000;
	}
}

internal BOOL CALLBACK
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
	
	if (DI_OK != IDirectInputDevice8_SetCooperativeLevel(gamepad->dinput.device, global_window, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))
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
	
	if (ConvertGuidToSDLGuid(instance, guid_str, sizeof guid_str))
	{
		for (int32 i = 0; i < ArrayLength(global_gamepadmap_database); ++i) {
			const GamepadMappings* map = &global_gamepadmap_database[i];
			
			if (MemCmp(guid_str, map->guid, ArrayLength(map->guid)) == 0)
			{
				gamepad->dinput.map = map;
				index_of_map_being_used = i;
				break;
			}
		}
	}
	
	Platform_DebugLog("Device Connected:\n");
	Platform_DebugLog("\tIndex: %i\n", index);
	Platform_DebugLog("\tDriver: DirectInput\n");
	Platform_DebugLog("\tName: %ls\n", instance->tszInstanceName);
	Platform_DebugLog("\tProduct Name: %ls\n", instance->tszProductName);
	Platform_DebugLog("\tGUID Instance: %08lx-%04hx-%04hx-%04hx-%04hx-%08lx\n",
					  guid->Data1, guid->Data2, guid->Data3,
					  *(uint16*)guid->Data4, ((uint16*)guid->Data4)[1], *(uint32*)guid->Data4);
	Platform_DebugLog("\tGUID Product: %08lx-%04hx-%04hx-%04hx-%04hx-%08lx\n",
					  instance->guidProduct.Data1, instance->guidProduct.Data2, instance->guidProduct.Data3,
					  *(uint16*)instance->guidProduct.Data4, ((uint16*)instance->guidProduct.Data4)[1], *(uint32*)instance->guidProduct.Data4);
	
	if (index_of_map_being_used == -1)
		Platform_DebugLog("\tMappings: Default\n");
	else
		Platform_DebugLog("\tMappings: Index %i\n", index_of_map_being_used);
	
	Platform_DebugLog("\tAxes: { ");
	for (int32 i = 0; i < gamepad->dinput.axis_count; ++i)
		Platform_DebugLog("%02x, ", (uint32)gamepad->dinput.axes[i]);
	Platform_DebugLog("}\n");
	
	Platform_DebugLog("\tButtons: { ");
	for (int32 i = 0; i < gamepad->dinput.button_count; ++i)
		Platform_DebugLog("%02x, ", (uint32)gamepad->dinput.buttons[i]);
	Platform_DebugLog("}\n");
	
	Platform_DebugLog("\tPOVs: { ");
	for (int32 i = 0; i < gamepad->dinput.pov_count; ++i)
		Platform_DebugLog("%02x, ", (uint32)gamepad->dinput.povs[i]);
	Platform_DebugLog("}\n");
	
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
internal void
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
		bool32 already_exists = false;
		
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
			
			Platform_DebugLog("Device Connected:\n");
			Platform_DebugLog("\tIndex: %i\n", index);
			Platform_DebugLog("\tDriver: XInput\n");
			Platform_DebugLog("\tSubtype: %s Controller\n", GetXInputSubTypeString(cap.SubType));
		}
	}
}

internal void
Win32_InitInput(void)
{
	Trace();
	
	// NOTE(ljre): Fill 'free spaces stack'
	global_gamepad_free_count = ArrayLength(global_gamepads);
	for (int32 i = 0; i < ArrayLength(global_gamepads); ++i)
	{
		global_gamepad_free[(int32)ArrayLength(global_gamepads) - i - 1] = i;
	}
	
	LoadXInput();
	LoadDirectInput();
	
	Win32_CheckForGamepads();
}

internal void
Win32_UpdateKeyboardKey(Engine_InputData* out_input_data, uint32 vkcode, bool is_down)
{
	Engine_KeyboardKey key = global_keyboard_key_table[vkcode];
	
	if (key)
	{
		out_input_data->keyboard.buttons[key].changes += 1;
		out_input_data->keyboard.buttons[key].is_down = is_down;
		
		if (out_input_data->keyboard.buffered_count < ArrayLength(out_input_data->keyboard.buffered))
			out_input_data->keyboard.buffered[out_input_data->keyboard.buffered_count++] = (uint8)key;
	}
}

internal void
Win32_UpdateMouseButton(Engine_InputData* out_input_data, Engine_MouseButton button, bool is_down)
{
	out_input_data->mouse.buttons[button].changes += 1;
	out_input_data->mouse.buttons[button].is_down = is_down;
}

internal inline void
Win32_UpdateInputPre(Engine_InputData* out_input_data)
{
	// NOTE(ljre): Update Keyboard
	for (int32 i = 0; i < ArrayLength(out_input_data->keyboard.buttons); ++i)
		out_input_data->keyboard.buttons[i].changes = 0;
	
	out_input_data->keyboard.buffered_count = 0;
	
	// NOTE(ljre): Update Mouse
	for (int32 i = 0; i < ArrayLength(out_input_data->mouse.buttons); ++i)
		out_input_data->mouse.buttons[i].changes = 0;
	
	out_input_data->mouse.scroll = 0;
}

internal inline void
Win32_UpdateInputPos(Engine_InputData* out_input_data)
{
	// NOTE(ljre): Get mouse cursor
	{
		out_input_data->mouse.old_pos[0] = out_input_data->mouse.pos[0];
		out_input_data->mouse.old_pos[1] = out_input_data->mouse.pos[1];
		
		POINT mouse;
		GetCursorPos(&mouse);
		ScreenToClient(global_window, &mouse);
		
		out_input_data->mouse.pos[0] = glm_clamp((float32)mouse.x, 0.0f, (float32)global_window_width);
		out_input_data->mouse.pos[1] = glm_clamp((float32)mouse.y, 0.0f, (float32)global_window_height);
		
		if (global_lock_cursor && GetActiveWindow())
		{
			mouse.x = global_window_width/2;
			mouse.y = global_window_height/2;
			
			ClientToScreen(global_window, &mouse);
			SetCursorPos(mouse.x, mouse.y);
			
			out_input_data->mouse.old_pos[0] = (float32)(global_window_width/2);
			out_input_data->mouse.old_pos[1] = (float32)(global_window_height/2);
		}
	}
	
	// NOTE(ljre): Update Gamepads
	out_input_data->connected_gamepads = 0;
	
	for (int32 i = 0; i < ArrayLength(global_gamepads); ++i)
	{
		Win32_Gamepad* gamepad = &global_gamepads[i];
		
		if (gamepad->connected)
		{
			out_input_data->connected_gamepads |= 1 << i;
			
			UpdateConnectedGamepad(gamepad);
			out_input_data->gamepads[i] = gamepad->data;
		}
		else
			MemSet(&out_input_data->gamepads[i], 0, sizeof(Engine_GamepadState));
	}
}
