#define MAX_GAMEPAD_COUNT 8
#define GAMEPAD_DEADZONE 0.4f

//~ Types and Macros
enum GamepadAPI
{
	GamepadAPI_None = 0,
	GamepadAPI_XInput = 1,
	GamepadAPI_DirectInput = 2,
} typedef GamepadAPI;

struct Win32_Gamepad
{
	GamepadAPI api;
	bool32 connected;
	Input_Gamepad data;
	
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
			
			int32 axes[6];
			int32 axis_count;
			int32 slider_count;
			int32 buttons[32];
			int32 button_count;
			int32 povs[4];
			int32 pov_count;
		} dinput;
	};
} typedef Win32_Gamepad;

#define XInputGetState global_proc_XInputGetState
#define XInputSetState global_proc_XInputSetState
#define DirectInput8Create global_proc_DirectInput8Create

typedef DWORD WINAPI ProcXInputGetState(DWORD index, XINPUT_STATE* state);
typedef DWORD WINAPI ProcXInputSetState(DWORD index, XINPUT_VIBRATION* vibration);
typedef HRESULT WINAPI ProcDirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);

//~ Globals
internal bool8           global_keyboard_keys[Input_KeyboardKey_Count];
internal Input_Mouse     global_mouse;
internal Win32_Gamepad   global_gamepads[MAX_GAMEPAD_COUNT];
internal int32           global_gamepad_free[MAX_GAMEPAD_COUNT];
internal int32           global_gamepad_free_count;
internal IDirectInput8W* global_dinput;
internal bool32          global_dinput_enabled;

internal ProcXInputGetState*     XInputGetState;
internal ProcXInputSetState*     XInputSetState;
internal ProcDirectInput8Create* DirectInput8Create;

internal Input_KeyboardKey global_keyboard_key_table[] = {
	[0] = Input_KeyboardKey_Any,
	
	[VK_ESCAPE] = Input_KeyboardKey_Escape,
	[VK_CONTROL] = Input_KeyboardKey_Control,
	[VK_SHIFT] = Input_KeyboardKey_Shift,
	[VK_MENU] = Input_KeyboardKey_Alt,
	[VK_TAB] = Input_KeyboardKey_Tab,
	[VK_RETURN] = Input_KeyboardKey_Enter,
	[VK_BACK] = Input_KeyboardKey_Backspace,
	[VK_UP] = Input_KeyboardKey_Up,
	[VK_DOWN] = Input_KeyboardKey_Down,
	[VK_LEFT] = Input_KeyboardKey_Left,
	[VK_RIGHT] = Input_KeyboardKey_Right,
	[VK_LCONTROL] = Input_KeyboardKey_LeftControl,
	[VK_RCONTROL] = Input_KeyboardKey_RightControl,
	[VK_LSHIFT] = Input_KeyboardKey_LeftShift,
	[VK_RSHIFT] = Input_KeyboardKey_RightShift,
	[VK_PRIOR] = Input_KeyboardKey_PageUp,
	[VK_NEXT] = Input_KeyboardKey_PageDown,
	[VK_END] = Input_KeyboardKey_End,
	[VK_HOME] = Input_KeyboardKey_Home,
	
	[VK_SPACE] = Input_KeyboardKey_Space,
	
	['0'] = Input_KeyboardKey_0,
	Input_KeyboardKey_1, Input_KeyboardKey_2, Input_KeyboardKey_3, Input_KeyboardKey_4,
	Input_KeyboardKey_5, Input_KeyboardKey_6, Input_KeyboardKey_7, Input_KeyboardKey_8,
	Input_KeyboardKey_9,
	
	['A'] = Input_KeyboardKey_A,
	Input_KeyboardKey_B, Input_KeyboardKey_C, Input_KeyboardKey_D, Input_KeyboardKey_E,
	Input_KeyboardKey_F, Input_KeyboardKey_G, Input_KeyboardKey_H, Input_KeyboardKey_I,
	Input_KeyboardKey_J, Input_KeyboardKey_K, Input_KeyboardKey_L, Input_KeyboardKey_M,
	Input_KeyboardKey_N, Input_KeyboardKey_O, Input_KeyboardKey_P, Input_KeyboardKey_Q,
	Input_KeyboardKey_R, Input_KeyboardKey_S, Input_KeyboardKey_T, Input_KeyboardKey_U,
	Input_KeyboardKey_V, Input_KeyboardKey_W, Input_KeyboardKey_X, Input_KeyboardKey_Y,
	Input_KeyboardKey_Z,
	
	[VK_F1] = Input_KeyboardKey_F1,
	Input_KeyboardKey_F2, Input_KeyboardKey_F3,  Input_KeyboardKey_F4,  Input_KeyboardKey_F4,
	Input_KeyboardKey_F5, Input_KeyboardKey_F6,  Input_KeyboardKey_F7,  Input_KeyboardKey_F8,
	Input_KeyboardKey_F9, Input_KeyboardKey_F10, Input_KeyboardKey_F11, Input_KeyboardKey_F12,
	Input_KeyboardKey_F13,
	
	[VK_NUMPAD0] = Input_KeyboardKey_Numpad0,
	Input_KeyboardKey_Numpad1, Input_KeyboardKey_Numpad2, Input_KeyboardKey_Numpad3, Input_KeyboardKey_Numpad4,
	Input_KeyboardKey_Numpad5, Input_KeyboardKey_Numpad6, Input_KeyboardKey_Numpad7, Input_KeyboardKey_Numpad8,
	Input_KeyboardKey_Numpad9,
};

//~ Not Mine!!
// Borrowed from GLFW with modifications.
//
// Original: https://github.com/glfw/glfw/blob/6876cf8d7e0e70dc3e4d7b0224d08312c9f78099/src/win32_joystick.c#L193
// License: https://github.com/glfw/glfw/blob/6876cf8d7e0e70dc3e4d7b0224d08312c9f78099/LICENSE.md
//
internal bool32 IsXInputDevice(const GUID* guid)
{
    RAWINPUTDEVICELIST ridl[512];
    UINT count = ArrayLength(ridl);
	
	count = GetRawInputDeviceList(ridl, &count, sizeof(RAWINPUTDEVICELIST));
    if (count == (UINT)-1)
        return false;
	
    for (UINT i = 0; i < count; i++)
    {
        RID_DEVICE_INFO rdi = { .cbSize = sizeof rdi, };
        char name[256] = { 0 };
        UINT rdi_size = sizeof rdi;
		UINT name_size = sizeof name;
		
        if (RIM_TYPEHID != ridl[i].dwType)
			continue;
		if ((UINT)-1 == GetRawInputDeviceInfoA(ridl[i].hDevice, RIDI_DEVICEINFO, &rdi, &rdi_size))
			continue;
		if (guid->Data1 != MAKELONG(rdi.hid.dwVendorId, rdi.hid.dwProductId))
            continue;
		
        if ((UINT)-1 == GetRawInputDeviceInfoA(ridl[i].hDevice, RIDI_DEVICENAME, name, &name_size))
            break;
		
        name[sizeof name - 1] = '\0';
        if (strstr(name, "IG_"))
			return true;
    }
	
    return false;
}

//~ Functions
internal bool32
AreGUIDsEqual(const GUID* a, const GUID* b)
{
	return memcmp(a, b, sizeof (GUID)) == 0;
}

internal int32
GenGamepadIndex(void)
{
	return global_gamepad_free[--global_gamepad_free_count];
}

internal void
ReleaseGamepadIndex(int32 index)
{
	global_gamepad_free[global_gamepad_free_count++] = index;
}

internal DWORD WINAPI
XInputGetStateStub(DWORD index, XINPUT_STATE* state) { return ERROR_DEVICE_NOT_CONNECTED; }
internal DWORD WINAPI
XInputSetStateStub(DWORD index, XINPUT_VIBRATION* state) { return ERROR_DEVICE_NOT_CONNECTED; }

internal void
LoadXInput(void)
{
	static const char* const dll_names[] = { "xinput1_4.dll", "xinput9_1_0.dll", "xinput1_3.dll" };
	
	XInputGetState = XInputGetStateStub;
	XInputSetState = XInputSetStateStub;
	
	for (int32 i = 0; i < ArrayLength(dll_names); ++i)
	{
		HMODULE library = LoadLibrary(dll_names[i]);
		
		if (library)
		{
			XInputGetState = (void*)GetProcAddress(library, "XInputGetState");
			XInputSetState = (void*)GetProcAddress(library, "XInputSetState");
			
			break;
		}
	}
}

internal void
LoadDirectInput(void)
{
	static const char* const dll_names[] = { "dinput8.dll", /* "dinput.dll" */ };
	
	bool32 loaded = false;
	for (int32 i = 0; i < ArrayLength(dll_names); ++i)
	{
		HMODULE library = LoadLibrary(dll_names[i]);
		
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
UpdateConnectedGamepad(Win32_Gamepad* gamepad)
{
	int32 index = (int32)(gamepad - global_gamepads) / (int32)(sizeof *gamepad);
	
	switch (gamepad->api)
	{
		//~ DirectInput
		case GamepadAPI_DirectInput:
		{
			DIJOYSTATE state;
			IDirectInputDevice8_Poll(gamepad->dinput.device);
			
			HRESULT result = IDirectInputDevice8_GetDeviceState(gamepad->dinput.device, sizeof state, &state);
			if (result == DIERR_NOTACQUIRED || result == DIERR_INPUTLOST)
			{
				IDirectInputDevice8_Acquire(gamepad->dinput.device);
				IDirectInputDevice8_Poll(gamepad->dinput.device);
				
				result = IDirectInputDevice8_GetDeviceState(gamepad->dinput.device, sizeof state, &state);
			}
			
			if (result == DI_OK)
			{
#if 1
				// NOTE(ljre): Debug Only
				if (Input_KeyboardIsPressed(Input_KeyboardKey_Space))
				{
					Platform_DebugLog("%5li %5li %5li ", state.lX, state.lY, state.lZ);
					Platform_DebugLog("%5li %5li %5li ", state.lRx, state.lRy, state.lRz);
					Platform_DebugLog("%5li %5li ", state.rglSlider[0], state.rglSlider[1]);
					Platform_DebugLog("%5li %5li %5li %5li ", state.rgdwPOV[0], state.rgdwPOV[1], state.rgdwPOV[2], state.rgdwPOV[3]);
					for (int32 i = 0; i < ArrayLength(state.rgbButtons); ++i)
						Platform_DebugLog("%c", (state.rgbButtons[i] & 128) ? '1' : '0');
					Platform_DebugLog("\n");
				}
#endif
				
				for (int32 i = 0; i < ArrayLength(gamepad->data.buttons); ++i)
				{
					bool8 button = gamepad->data.buttons[i];
					
					button = (bool8)(button << 1) | (button & 1);
					
					gamepad->data.buttons[i] = button;
				}
			}
			else
			{
				IDirectInputDevice8_Unacquire(gamepad->dinput.device);
				IDirectInputDevice8_Release(gamepad->dinput.device);
				
				gamepad->connected = false;
				gamepad->api = GamepadAPI_None;
				ReleaseGamepadIndex(index);
			}
		} break;
		
		//~ XInput
		case GamepadAPI_XInput:
		{
			XINPUT_STATE state;
			DWORD result = XInputGetState(gamepad->xinput.index, &state);
			
			if (result == ERROR_SUCCESS)
			{
				//- Left Stick
				gamepad->data.left[0] = ((float32)state.Gamepad.sThumbLX + 0.5f) /  32767.5f;
				gamepad->data.left[1] = ((float32)state.Gamepad.sThumbLY + 0.5f) / -32767.5f;
				
				// NOTE(ljre): normalize
				{
					float32 dir = atan2f(gamepad->data.left[1], gamepad->data.left[0]);
					float32 mag = sqrtf(gamepad->data.left[0]*gamepad->data.left[0] +
										gamepad->data.left[1]*gamepad->data.left[1]);
					
					if (mag < GAMEPAD_DEADZONE)
					{
						gamepad->data.left[0] = 0.0f;
						gamepad->data.left[1] = 0.0f;
					}
					else
					{
						gamepad->data.left[0] = cosf(dir);
						gamepad->data.left[1] = sinf(dir);
					}
				}
				
				//- Right Stick
				gamepad->data.right[0] = ((float32)state.Gamepad.sThumbRX + 0.5f) /  32767.5f;
				gamepad->data.right[1] = ((float32)state.Gamepad.sThumbRY + 0.5f) / -32767.5f;
				
				// NOTE(ljre): normalize
				{
					float32 dir = atan2f(gamepad->data.right[1], gamepad->data.right[0]);
					float32 mag = sqrtf(gamepad->data.right[0]*gamepad->data.right[0] +
										gamepad->data.right[1]*gamepad->data.right[1]);
					
					if (mag < GAMEPAD_DEADZONE)
					{
						gamepad->data.right[0] = 0.0f;
						gamepad->data.right[1] = 0.0f;
					}
					else
					{
						gamepad->data.right[0] = cosf(dir);
						gamepad->data.right[1] = sinf(dir);
					}
				}
				
				//- LR & RB
				gamepad->data.lt = (float32)state.Gamepad.bLeftTrigger / 255.0f;
				gamepad->data.rt = (float32)state.Gamepad.bRightTrigger / 255.0f;
				
				//- Buttons
#define _SetButton(_i, _v)\
gamepad->data.buttons[_i] <<= 1;\
gamepad->data.buttons[_i] |= !!(state.Gamepad.wButtons & _v)
				
				_SetButton(Input_GamepadButton_Left, XINPUT_GAMEPAD_DPAD_LEFT);
				_SetButton(Input_GamepadButton_Right, XINPUT_GAMEPAD_DPAD_RIGHT);
				_SetButton(Input_GamepadButton_Up, XINPUT_GAMEPAD_DPAD_UP);
				_SetButton(Input_GamepadButton_Down, XINPUT_GAMEPAD_DPAD_DOWN);
				_SetButton(Input_GamepadButton_Start, XINPUT_GAMEPAD_START);
				_SetButton(Input_GamepadButton_Back, XINPUT_GAMEPAD_BACK);
				_SetButton(Input_GamepadButton_LS, XINPUT_GAMEPAD_LEFT_THUMB);
				_SetButton(Input_GamepadButton_RS, XINPUT_GAMEPAD_RIGHT_THUMB);
				_SetButton(Input_GamepadButton_LB, XINPUT_GAMEPAD_LEFT_SHOULDER);
				_SetButton(Input_GamepadButton_RB, XINPUT_GAMEPAD_RIGHT_SHOULDER);
				_SetButton(Input_GamepadButton_A, XINPUT_GAMEPAD_A);
				_SetButton(Input_GamepadButton_B, XINPUT_GAMEPAD_B);
				_SetButton(Input_GamepadButton_X, XINPUT_GAMEPAD_X);
				_SetButton(Input_GamepadButton_Y, XINPUT_GAMEPAD_Y);
				
#undef _SetButton
			}
			else if (result == ERROR_DEVICE_NOT_CONNECTED)
			{
				// NOTE(ljre): Controller is not connected!
				gamepad->api = GamepadAPI_None;
				gamepad->connected = false;
				ReleaseGamepadIndex(index);
			}
		} break;
		
		case GamepadAPI_None:
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
				.dwSize = sizeof range_property,
				.dwHeaderSize = sizeof range_property.diph,
				.dwObj = object->dwType,
				.dwHow = DIPH_BYID,
			},
			.lMin = INT16_MIN,
			.lMax = INT16_MAX,
		};
		
		if (0) {}
		else if (AreGUIDsEqual(object_guid, &GUID_Slider)) axis_offset = (int32)DIJOFS_SLIDER(gamepad->dinput.slider_count++);
        else if (AreGUIDsEqual(object_guid, &GUID_XAxis )) axis_offset = (int32)DIJOFS_X;
        else if (AreGUIDsEqual(object_guid, &GUID_YAxis )) axis_offset = (int32)DIJOFS_Y;
        else if (AreGUIDsEqual(object_guid, &GUID_ZAxis )) axis_offset = (int32)DIJOFS_Z;
        else if (AreGUIDsEqual(object_guid, &GUID_RxAxis)) axis_offset = (int32)DIJOFS_RX;
        else if (AreGUIDsEqual(object_guid, &GUID_RyAxis)) axis_offset = (int32)DIJOFS_RY;
        else if (AreGUIDsEqual(object_guid, &GUID_RzAxis)) axis_offset = (int32)DIJOFS_RZ;
		else return DIENUM_CONTINUE;
		
		if (DI_OK != IDirectInputDevice8_SetProperty(gamepad->dinput.device, DIPROP_RANGE, &range_property.diph))
			return DIENUM_CONTINUE;
		
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

internal BOOL CALLBACK
DirectInputEnumDevicesCallback(const DIDEVICEINSTANCEW* instance, void* userdata)
{
	if (global_gamepad_free_count <= 0)
		return DIENUM_STOP;
	
	const GUID* guid = &instance->guidInstance;
	if (IsXInputDevice(guid))
		return DIENUM_CONTINUE;
	
	// Check if device is already added
	for (int32 i = 0; i < ArrayLength(global_gamepads); ++i)
	{
		if (global_gamepads[i].connected &&
			global_gamepads[i].api == GamepadAPI_DirectInput &&
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
		.dwSize = sizeof capabilities,
	};
	DIPROPDWORD word_property = {
		.diph = {
			.dwSize = sizeof word_property,
			.dwHeaderSize = sizeof word_property.diph,
			.dwObj = 0,
			.dwHow = DIPH_DEVICE,
		},
		.dwData = DIPROPAXISMODE_ABS,
	};
	
	if (DI_OK != IDirectInput8_CreateDevice(global_dinput, guid, &gamepad->dinput.device, NULL))
		goto _failure;
	
	if (DI_OK != IDirectInputDevice8_SetCooperativeLevel(gamepad->dinput.device, global_window, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))
		goto _failure;
	
	if (DI_OK != IDirectInputDevice8_SetDataFormat(gamepad->dinput.device, &c_dfDIJoystick))
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
	gamepad->api = GamepadAPI_DirectInput;
	gamepad->dinput.guid = *guid;
	
	Platform_DebugLog("Device Connected:\n");
	Platform_DebugLog("\tIndex: %i\n", index);
	Platform_DebugLog("\tName: %ls\n", instance->tszInstanceName);
	Platform_DebugLog("\tProduct Name: %ls\n", instance->tszProductName);
	Platform_DebugLog("\tGUID: %lx-%hx-%hx-%hx-%hx-%lx\n",
					  guid->Data1, guid->Data2, guid->Data3,
					  *(uint16*)guid->Data4, ((uint16*)guid->Data4)[1], *(uint32*)guid->Data4);
	
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
		gamepad->api = GamepadAPI_None;
		IDirectInputDevice8_Release(gamepad->dinput.device);
		ReleaseGamepadIndex(index);
		
		return DIENUM_CONTINUE;
	}
}

//~ Internal API
internal void
Win32_CheckForGamepads(void)
{
	// NOTE(ljre): Don't check for more devices if there isn't more space
	if (global_gamepad_free_count <= 0)
		return;
	
	// Direct Input
	IDirectInput8_EnumDevices(global_dinput, DI8DEVCLASS_GAMECTRL, DirectInputEnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
	
	// XInput
	for (DWORD i = 0; i < 4 && global_gamepad_free_count > 0; ++i)
	{
		bool32 already_exists = false;
		
		for (int32 index = 0; index < ArrayLength(global_gamepads); ++index)
		{
			if (global_gamepads[index].connected &&
				global_gamepads[index].api == GamepadAPI_XInput &&
				global_gamepads[index].xinput.index == i)
			{
				already_exists = true;
			}
		}
		
		XINPUT_STATE dummy;
		if (!already_exists && XInputGetState(i, &dummy) == ERROR_SUCCESS)
		{
			int32 index = GenGamepadIndex();
			Win32_Gamepad* gamepad = &global_gamepads[index];
			
			gamepad->connected = true;
			gamepad->api = GamepadAPI_XInput;
			gamepad->xinput.index = i;
		}
	}
}

internal void
Win32_InitInput(void)
{
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
Win32_UpdateInputPre(void)
{
	// NOTE(ljre): Update Keyboard
	{
		for (int32 i = 0; i < ArrayLength(global_keyboard_keys); ++i)
		{
			bool8 key = global_keyboard_keys[i];
			
			key = (bool8)(key << 1) | (key & 1);
			
			global_keyboard_keys[i] = key;
		}
	}
}

internal void
Win32_UpdateInputPos(void)
{
	// NOTE(ljre): Get mouse cursor
	{
		global_mouse.old_pos[0] = global_mouse.pos[0];
		global_mouse.old_pos[1] = global_mouse.pos[1];
		
		POINT mouse;
		GetCursorPos(&mouse);
		ScreenToClient(global_window, &mouse);
		
		global_mouse.pos[0] = (float32)mouse.x;
		global_mouse.pos[1] = (float32)mouse.x;
	}
	
	// NOTE(ljre): Update Gamepads
	{
		for (int32 i = 0; i < ArrayLength(global_gamepads); ++i)
		{
			Win32_Gamepad* gamepad = &global_gamepads[i];
			
			if (gamepad->connected)
				UpdateConnectedGamepad(gamepad);
		}
	}
}

//~ API
API bool32
Input_KeyboardIsPressed(int32 key)
{
	if (key < 0 || key >= ArrayLength(global_keyboard_keys))
		return false;
	
	return !(global_keyboard_keys[key] & 2) && (global_keyboard_keys[key] & 1);
}

API bool32
Input_KeyboardIsDown(int32 key)
{
	if (key < 0 || key >= ArrayLength(global_keyboard_keys))
		return false;
	
	return (global_keyboard_keys[key] & 1);
}

API bool32
Input_KeyboardIsReleased(int32 key)
{
	if (key < 0 || key >= ArrayLength(global_keyboard_keys))
		return false;
	
	return (global_keyboard_keys[key] & 2) && !(global_keyboard_keys[key] & 1);
}

API bool32
Input_KeyboardIsUp(int32 key)
{
	if (key < 0 || key >= ArrayLength(global_keyboard_keys))
		return false;
	
	return !(global_keyboard_keys[key] & 1);
}

API const Input_Mouse*
Input_GetMouse(void)
{
	return &global_mouse;
}

API const Input_Gamepad*
Input_GetGamepad(int32 index)
{
	if (index < 0 || index >= ArrayLength(global_gamepads))
		return NULL;
	
	Win32_Gamepad* gamepad = &global_gamepads[index];
	if (!gamepad->connected)
		return NULL;
	
	return &gamepad->data;
}

API void
Input_SetGamepad(int32 index, float32 vibration)
{
	if (index < 0 || index >= ArrayLength(global_gamepads))
		return;
	
	Win32_Gamepad* gamepad = &global_gamepads[index];
	if (!gamepad->connected)
		return;
	
	switch (gamepad->api)
	{
		case GamepadAPI_DirectInput:
		{
			// TODO
		} break;
		
		case GamepadAPI_XInput:
		{
			WORD speed = (WORD)(vibration * 65.535f);
			
			XINPUT_VIBRATION vibration = {
				.wLeftMotorSpeed = speed,
				.wRightMotorSpeed = speed
			};
			
			XInputSetState(gamepad->xinput.index, &vibration);
		} break;
		
		case GamepadAPI_None:
		{
		} break;
	}
}
