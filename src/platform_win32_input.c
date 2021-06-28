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
		} dinput;
	};
} typedef Win32_Gamepad;

#define XInputGetState Win32_ProcXInputGetState
#define XInputSetState Win32_ProcXInputSetState
#define DirectInput8Create Win32_DirectInput8Create

typedef DWORD WINAPI Win32_FnXInputGetState(DWORD index, XINPUT_STATE* state);
typedef DWORD WINAPI Win32_FnXInputSetState(DWORD index, XINPUT_VIBRATION* vibration);
typedef HRESULT WINAPI Win32_FnDirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);

//~ Globals
internal bool8 global_keyboard_keys[Input_KeyboardKey_Count];
internal Input_Mouse global_mouse;
internal Win32_Gamepad global_gamepads[MAX_GAMEPAD_COUNT];
internal int32 global_gamepad_free[MAX_GAMEPAD_COUNT];
internal int32 global_gamepad_free_count;
internal IDirectInput8W* global_dinput;
internal bool32 global_dinput_enabled;

internal Win32_FnXInputGetState* XInputGetState;
internal Win32_FnXInputSetState* XInputSetState;
internal Win32_FnDirectInput8Create* DirectInput8Create;

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

//~ Forward Declarations
internal void Win32_CheckForGamepads(void);
internal BOOL IsXInputDevice(const GUID* pGuidProductFromDirectInput);

//~ Functions
internal bool32
Win32_AreGUIDsEqual(const GUID* a, const GUID* b)
{
	bool32 result = false;
	
	result = result && (a->Data1 == b->Data1);
	result = result && (a->Data2 == b->Data2);
	result = result && (a->Data3 == b->Data3);
	result = result && (memcmp(a->Data4, b->Data4, sizeof a->Data4) == 0);
	
	return result;
}

internal int32
Win32_GenGamepadIndex(void)
{
	return global_gamepad_free[--global_gamepad_free_count];
}

internal void
Win32_ReleaseGamepadIndex(int32 index)
{
	global_gamepad_free[global_gamepad_free_count++] = index;
}

DWORD WINAPI XInputGetStateStub(DWORD index, XINPUT_STATE* state) { return ERROR_DEVICE_NOT_CONNECTED; }
DWORD WINAPI XInputSetStateStub(DWORD index, XINPUT_VIBRATION* state) { return ERROR_DEVICE_NOT_CONNECTED; }

internal void
Win32_LoadXInput(void)
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
Win32_LoadDirectInput(void)
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
Win32_InitInput(void)
{
	// NOTE(ljre): Fill 'free spaces stack'
	global_gamepad_free_count = ArrayLength(global_gamepads);
	for (int32 i = 0; i < ArrayLength(global_gamepads); ++i)
	{
		global_gamepad_free[(int32)ArrayLength(global_gamepads) - i - 1] = i;
	}
	
	Win32_LoadXInput();
	Win32_LoadDirectInput();
	
	Win32_CheckForGamepads();
}

internal void
Win32_UpdateConnectedGamepad(Win32_Gamepad* gamepad, int32 index)
{
	for (int32 i = 0; i < ArrayLength(gamepad->data.buttons); ++i)
	{
		bool8 button = gamepad->data.buttons[i];
		
		button = (bool8)(button << 1) | (button & 1);
		
		gamepad->data.buttons[i] = button;
	}
	
	switch (gamepad->api)
	{
		//~ DirectInput
		case GamepadAPI_DirectInput:
		{
			DIJOYSTATE state;
			IDirectInputDevice8_Poll(gamepad->dinput.device);
			
			if (DI_OK == IDirectInputDevice8_GetDeviceState(gamepad->dinput.device, sizeof state, &state))
			{
				
			}
			else
			{
				IDirectInputDevice8_Unacquire(gamepad->dinput.device);
				IDirectInputDevice8_Release(gamepad->dinput.device);
				
				gamepad->connected = false;
				gamepad->api = GamepadAPI_None;
				Win32_ReleaseGamepadIndex(index);
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
				if (state.Gamepad.sThumbLX < 0)
					gamepad->data.left[0] = (float32)state.Gamepad.sThumbLX / 32768.0f;
				else gamepad->data.left[0] = (float32)state.Gamepad.sThumbLX / 32767.0f;
				
				if (state.Gamepad.sThumbLY < 0)
					gamepad->data.left[1] = -(float32)state.Gamepad.sThumbLY / 32768.0f;
				else gamepad->data.left[1] = -(float32)state.Gamepad.sThumbLY / 32767.0f;
				
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
				if (state.Gamepad.sThumbRX < 0)
					gamepad->data.right[0] = (float32)state.Gamepad.sThumbRX / 32768.0f;
				else gamepad->data.right[0] = (float32)state.Gamepad.sThumbRX / 32767.0f;
				
				if (state.Gamepad.sThumbRY < 0)
					gamepad->data.right[1] = -(float32)state.Gamepad.sThumbRY / 32768.0f;
				else gamepad->data.right[1] = -(float32)state.Gamepad.sThumbRY / 32767.0f;
				
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
gamepad->data.buttons[_i] &= ~1;\
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
			else
			{
				// NOTE(ljre): Controller is not connected!
				gamepad->api = GamepadAPI_None;
				gamepad->connected = false;
			}
		} break;
		
		case GamepadAPI_None:
		{
			// NOTE(ljre): This should be unreachable... just to be safe
			gamepad->connected = false;
		}
		
		default: //~ Unknown
		{
			Platform_DebugLog("[ERROR] Unknown GamepadAPI value: %i\n", gamepad->api);
		} break;
	}
}

internal BOOL
DirectInputEnumDevicesCallback(LPCDIDEVICEINSTANCEW instance, LPVOID userdata)
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
			Win32_AreGUIDsEqual(guid, &global_gamepads[i].dinput.guid))
		{
			return DIENUM_CONTINUE;
		}
	}
	
	// It's not! Add it!
	int32 index = Win32_GenGamepadIndex();
	Win32_Gamepad* gamepad = &global_gamepads[index];
	
	if (DI_OK == IDirectInput8_CreateDevice(global_dinput, guid, &gamepad->dinput.device, NULL))
	{
		if (DI_OK == IDirectInputDevice8_SetDataFormat(gamepad->dinput.device, &c_dfDIJoystick) &&
			DI_OK == IDirectInputDevice8_Acquire(gamepad->dinput.device))
		{
			gamepad->connected = true;
			gamepad->api = GamepadAPI_DirectInput;
			gamepad->dinput.guid = *guid;
			
			return DIENUM_CONTINUE;
		}
		else
		{
			IDirectInputDevice8_Release(gamepad->dinput.device);
		}
	}
	
	// Failure
	gamepad->connected = false;
	gamepad->api = GamepadAPI_None;
	Win32_ReleaseGamepadIndex(index);
	
	return DIENUM_CONTINUE;
}

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
			int32 index = Win32_GenGamepadIndex();
			Win32_Gamepad* gamepad = &global_gamepads[index];
			
			gamepad->connected = true;
			gamepad->api = GamepadAPI_XInput;
			gamepad->xinput.index = i;
		}
	}
}

internal void
Win32_UpdateInput(void)
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
	
	// NOTE(ljre): Update Keyboard
	{
		for (int32 i = 0; i < ArrayLength(global_keyboard_keys); ++i)
		{
			bool8 key = global_keyboard_keys[i];
			
			key = (bool8)(key << 1) | (key & 1);
			
			global_keyboard_keys[i] = key;
		}
	}
	
	// NOTE(ljre): Update Gamepads
	{
		for (int32 i = 0; i < ArrayLength(global_gamepads); ++i)
		{
			Win32_Gamepad* gamepad = &global_gamepads[i];
			
			if (gamepad->connected)
				Win32_UpdateConnectedGamepad(gamepad, i);
		}
	}
}

//~ API
API bool32
Input_KeyboardIsPressed(int32 key)
{
	if (key < 0 || key >= ArrayLength(global_keyboard_keys))
		return false;
	
	return !(global_keyboard_keys[key] & 3) && (global_keyboard_keys[key] & 1);
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
	
	return (global_keyboard_keys[key] & 3) && !(global_keyboard_keys[key] & 1);
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

//~ Not Mine!!
//
//
// https://docs.microsoft.com/en-us/windows/win32/xinput/xinput-and-directinput
//
#include <wbemidl.h>
#include <oleauto.h>
//#include <wmsstd.h>
#define SAFE_RELEASE(T, p) { if ( (p) ) { T ## _Release(p); (p) = 0; } }

// NOTE(ljre): Define GUIDs... I don't know why the fuck I need those
DEFINE_GUID(CLSID_WbemLocator,
			0x4590f811, 0x1d3a, 0x11d0, 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);
DEFINE_GUID(IID_IWbemLocator,
			0xdc12a687, 0x737f, 0x11cf, 0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);

//-----------------------------------------------------------------------------
// Enum each PNP device using WMI and check each device ID to see if it contains 
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
// Unfortunately this information can not be found by just using DirectInput 
//-----------------------------------------------------------------------------
BOOL IsXInputDevice( const GUID* pGuidProductFromDirectInput )
{
    IWbemLocator*           pIWbemLocator  = NULL;
    IEnumWbemClassObject*   pEnumDevices   = NULL;
    IWbemClassObject*       pDevices[20]   = {0};
    IWbemServices*          pIWbemServices = NULL;
    BSTR                    bstrNamespace  = NULL;
    BSTR                    bstrDeviceID   = NULL;
    BSTR                    bstrClassName  = NULL;
    DWORD                   uReturned      = 0;
    bool32                  bIsXinputDevice= false;
    UINT                    iDevice        = 0;
    VARIANT                 var;
    HRESULT                 hr;
	
    // CoInit if needed
    hr = CoInitialize(NULL);
    bool32 bCleanupCOM = SUCCEEDED(hr);
	
    // So we can call VariantClear() later, even if we never had a successful IWbemClassObject::Get().
    VariantInit(&var);
	
    // Create WMI
    hr = CoCreateInstance( &CLSID_WbemLocator,
						  NULL,
						  CLSCTX_INPROC_SERVER,
						  __uuidof(IWbemLocator),
						  (LPVOID*) &pIWbemLocator);
    if( FAILED(hr) || pIWbemLocator == NULL )
        goto LCleanup;
	
    bstrNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );if( bstrNamespace == NULL ) goto LCleanup;        
    bstrClassName = SysAllocString( L"Win32_PNPEntity" );   if( bstrClassName == NULL ) goto LCleanup;        
    bstrDeviceID  = SysAllocString( L"DeviceID" );          if( bstrDeviceID == NULL )  goto LCleanup;        
    
    // Connect to WMI 
    hr = IWbemLocator_ConnectServer( pIWbemLocator, bstrNamespace, NULL, NULL, 0L, 
									0L, NULL, NULL, &pIWbemServices );
    if( FAILED(hr) || pIWbemServices == NULL )
        goto LCleanup;
	
    // Switch security level to IMPERSONATE. 
    CoSetProxyBlanket( (void*)pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
					  RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );                    
	
    hr = IWbemServices_CreateInstanceEnum( pIWbemServices, bstrClassName, 0, NULL, &pEnumDevices ); 
    if( FAILED(hr) || pEnumDevices == NULL )
        goto LCleanup;
	
    // Loop over all devices
    for( ;; )
    {
        // Get 20 at a time
        hr = IEnumWbemClassObject_Next( pEnumDevices, 10000, 20, pDevices, &uReturned );
        if( FAILED(hr) )
            goto LCleanup;
        if( uReturned == 0 )
            break;
		
        for( iDevice=0; iDevice<uReturned; iDevice++ )
        {
            // For each device, get its device ID
            hr = IWbemClassObject_Get( pDevices[iDevice], bstrDeviceID, 0L, &var, NULL, NULL );
            if( SUCCEEDED( hr ) && var.vt == VT_BSTR && var.bstrVal != NULL )
            {
                // Check if the device ID contains "IG_".  If it does, then it's an XInput device
				// This information can not be found from DirectInput 
                if( wcsstr( var.bstrVal, L"IG_" ) )
                {
                    // If it does, then get the VID/PID from var.bstrVal
                    DWORD dwPid = 0, dwVid = 0;
                    WCHAR* strVid = wcsstr( var.bstrVal, L"VID_" );
                    if( strVid && swscanf( strVid, L"VID_%4X", &dwVid ) != 1 )
                        dwVid = 0;
                    WCHAR* strPid = wcsstr( var.bstrVal, L"PID_" );
                    if( strPid && swscanf( strPid, L"PID_%4X", &dwPid ) != 1 )
                        dwPid = 0;
					
                    // Compare the VID/PID to the DInput device
                    DWORD dwVidPid = (DWORD)MAKELONG( dwVid, dwPid );
                    if( dwVidPid == pGuidProductFromDirectInput->Data1 )
                    {
                        bIsXinputDevice = true;
                        goto LCleanup;
                    }
                }
            }
            VariantClear(&var);
            SAFE_RELEASE( IWbemClassObject, pDevices[iDevice] );
        }
    }
	
	LCleanup:
    VariantClear(&var);
    if(bstrNamespace)
        SysFreeString(bstrNamespace);
    if(bstrDeviceID)
        SysFreeString(bstrDeviceID);
    if(bstrClassName)
        SysFreeString(bstrClassName);
    for( iDevice=0; iDevice<20; iDevice++ )
        SAFE_RELEASE( IWbemClassObject, pDevices[iDevice] );
    SAFE_RELEASE( IEnumWbemClassObject, pEnumDevices );
    SAFE_RELEASE( IWbemLocator, pIWbemLocator );
    SAFE_RELEASE( IWbemServices, pIWbemServices );
	
    if( bCleanupCOM )
        CoUninitialize();
	
    return bIsXinputDevice;
}
