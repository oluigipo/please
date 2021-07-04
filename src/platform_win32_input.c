#include "internal_gamepad_map.h"

#define MAX_GAMEPAD_COUNT 16
#define GAMEPAD_DEADZONE 0.3f

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
} typedef Win32_Gamepad;

#define XInputGetState global_proc_XInputGetState
#define XInputSetState global_proc_XInputSetState
#define XInputGetCapabilities global_proc_XInputGetCapabilities
#define DirectInput8Create global_proc_DirectInput8Create

typedef DWORD WINAPI ProcXInputGetState(DWORD index, XINPUT_STATE* state);
typedef DWORD WINAPI ProcXInputSetState(DWORD index, XINPUT_VIBRATION* vibration);
typedef DWORD WINAPI ProcXInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);
typedef HRESULT WINAPI ProcDirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);

//~ Globals
internal bool8           global_keyboard_keys[Input_KeyboardKey_Count];
internal Input_Mouse     global_mouse;
internal Win32_Gamepad   global_gamepads[MAX_GAMEPAD_COUNT];
internal int32           global_gamepad_free[MAX_GAMEPAD_COUNT];
internal int32           global_gamepad_free_count;
internal IDirectInput8W* global_dinput;
internal bool32          global_dinput_enabled;

internal ProcXInputGetState*        XInputGetState;
internal ProcXInputSetState*        XInputSetState;
internal ProcXInputGetCapabilities* XInputGetCapabilities;
internal ProcDirectInput8Create*    DirectInput8Create;

internal const Input_KeyboardKey global_keyboard_key_table[] = {
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
        
        name[sizeof(name) - 1] = '\0';
        if (strstr(name, "IG_"))
            return true;
    }
    
    return false;
}

//~ Functions
internal inline bool32
AreGUIDsEqual(const GUID* a, const GUID* b)
{
    return memcmp(a, b, sizeof (GUID)) == 0;
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
    float32 dir = atan2f(axis[1], axis[0]);
    float32 mag = sqrtf(axis[0]*axis[0] + axis[1]*axis[1]);
    
    if (mag < GAMEPAD_DEADZONE)
    {
        axis[0] = 0.0f;
        axis[1] = 0.0f;
    }
    else
    {
        axis[0] = cosf(dir) * mag;
        axis[1] = sinf(dir) * mag;
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
        HMODULE library = LoadLibraryA(dll_names[i]);
        
        if (library)
        {
            XInputGetState        = (void*)GetProcAddress(library, "XInputGetState");
            XInputSetState        = (void*)GetProcAddress(library, "XInputSetState");
            XInputGetCapabilities = (void*)GetProcAddress(library, "XInputGetCapabilities");
            
            Platform_DebugLog("Loaded Library: %s\n", dll_names[i]);
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
        HMODULE library = LoadLibraryA(dll_names[i]);
        
        if (library)
        {
            DirectInput8Create = (void*)GetProcAddress(library, "DirectInput8Create");
            Platform_DebugLog("Loaded Library: %s\n", dll_names[i]);
            
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
TranslateController(Input_Gamepad* out, const GamepadMappings* map,
                    const bool8 buttons[32], const float32 axes[16], const int32 povs[8])
{
    //- Update Buttons
    for (int32 i = 0; i < ArrayLength(out->buttons); ++i)
    {
        out->buttons[i] <<= 1;
    }
    
    //- Translate Buttons
    for (int32 i = 0; i < ArrayLength(map->buttons); ++i)
    {
        if (!map->buttons[i])
            continue;
        
        Input_GamepadButton as_button = -1;
        
        uint8 lower = map->buttons[i] & 255;
        //uint8 higher = map->buttons[i] >> 8;
        
        switch (lower)
        {
            case GamepadObject_Button_A: as_button = Input_GamepadButton_A; break;
            case GamepadObject_Button_B: as_button = Input_GamepadButton_B; break;
            case GamepadObject_Button_X: as_button = Input_GamepadButton_X; break;
            case GamepadObject_Button_Y: as_button = Input_GamepadButton_Y; break;
            case GamepadObject_Button_Left: as_button = Input_GamepadButton_Left; break;
            case GamepadObject_Button_Right: as_button = Input_GamepadButton_Right; break;
            case GamepadObject_Button_Up: as_button = Input_GamepadButton_Up; break;
            case GamepadObject_Button_Down: as_button = Input_GamepadButton_Down; break;
            case GamepadObject_Button_LeftShoulder: as_button = Input_GamepadButton_LB; break;
            case GamepadObject_Button_RightShoulder: as_button = Input_GamepadButton_RB; break;
            case GamepadObject_Button_LeftStick: as_button = Input_GamepadButton_LS; break;
            case GamepadObject_Button_RightStick: as_button = Input_GamepadButton_RS; break;
            case GamepadObject_Button_Start: as_button = Input_GamepadButton_Start; break;
            case GamepadObject_Button_Back: as_button = Input_GamepadButton_Back; break;
            
            case GamepadObject_Pressure_LeftTrigger: out->lt = (float32)(buttons[i] != 0); break;
            case GamepadObject_Pressure_RightTrigger: out->rt = (float32)(buttons[i] != 0); break;
            
            // NOTE(ljre): Every other is invalid
            default: break;
        }
        
        if (as_button != -1)
        {
            out->buttons[as_button] |= buttons[i];
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
        
        Input_GamepadButton as_button;
        int32 pov = povs[i];
        uint8 lower, higher;
        uint16 object;
        
        if (pov == -1)
            continue;
        
        as_button = -1;
        object = map->povs[i][pov >> 1];
        lower = object & 255;
        higher = object >> 8;
        switch (lower)
        {
            case GamepadObject_Button_A: as_button = Input_GamepadButton_A; break;
            case GamepadObject_Button_B: as_button = Input_GamepadButton_B; break;
            case GamepadObject_Button_X: as_button = Input_GamepadButton_X; break;
            case GamepadObject_Button_Y: as_button = Input_GamepadButton_Y; break;
            case GamepadObject_Button_Left: as_button = Input_GamepadButton_Left; break;
            case GamepadObject_Button_Right: as_button = Input_GamepadButton_Right; break;
            case GamepadObject_Button_Up: as_button = Input_GamepadButton_Up; break;
            case GamepadObject_Button_Down: as_button = Input_GamepadButton_Down; break;
            case GamepadObject_Button_LeftShoulder: as_button = Input_GamepadButton_LB; break;
            case GamepadObject_Button_RightShoulder: as_button = Input_GamepadButton_RB; break;
            case GamepadObject_Button_LeftStick: as_button = Input_GamepadButton_LS; break;
            case GamepadObject_Button_RightStick: as_button = Input_GamepadButton_RS; break;
            case GamepadObject_Button_Start: as_button = Input_GamepadButton_Start; break;
            case GamepadObject_Button_Back: as_button = Input_GamepadButton_Back; break;
            
            default: break;
        }
        
        if (as_button != -1)
        {
            out->buttons[as_button] |= 1;
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
            case GamepadObject_Button_A: as_button = Input_GamepadButton_A; break;
            case GamepadObject_Button_B: as_button = Input_GamepadButton_B; break;
            case GamepadObject_Button_X: as_button = Input_GamepadButton_X; break;
            case GamepadObject_Button_Y: as_button = Input_GamepadButton_Y; break;
            case GamepadObject_Button_Left: as_button = Input_GamepadButton_Left; break;
            case GamepadObject_Button_Right: as_button = Input_GamepadButton_Right; break;
            case GamepadObject_Button_Up: as_button = Input_GamepadButton_Up; break;
            case GamepadObject_Button_Down: as_button = Input_GamepadButton_Down; break;
            case GamepadObject_Button_LeftShoulder: as_button = Input_GamepadButton_LB; break;
            case GamepadObject_Button_RightShoulder: as_button = Input_GamepadButton_RB; break;
            case GamepadObject_Button_LeftStick: as_button = Input_GamepadButton_LS; break;
            case GamepadObject_Button_RightStick: as_button = Input_GamepadButton_RS; break;
            case GamepadObject_Button_Start: as_button = Input_GamepadButton_Start; break;
            case GamepadObject_Button_Back: as_button = Input_GamepadButton_Back; break;
            
            default: break;
        }
        
        if (as_button != -1)
        {
            out->buttons[as_button] |= 1;
        }
    }
    
    //- The end of your pain
    NormalizeAxis(out->left);
    NormalizeAxis(out->right);
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
            DIJOYSTATE2 state;
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
                
#ifdef DEBUG
                // NOTE(ljre): Debug Only
                if (Input_KeyboardIsPressed(Input_KeyboardKey_Space))
                {
                    Platform_DebugLog("Raw:");
                    Platform_DebugLog("%6li %6li %6li ", state.lX, state.lY, state.lZ);
                    Platform_DebugLog("%6li %6li %6li ", state.lRx, state.lRy, state.lRz);
                    Platform_DebugLog("%6li %6li ", state.rglSlider[0], state.rglSlider[1]);
                    Platform_DebugLog("%6li %6li %6li %6li ", state.rgdwPOV[0], state.rgdwPOV[1], state.rgdwPOV[2], state.rgdwPOV[3]);
                    for (int32 i = 0; i < ArrayLength(state.rgbButtons); ++i)
                        Platform_DebugLog("%c", (state.rgbButtons[i] & 128) ? '1' : '0');
                    Platform_DebugLog("\n");
                    
                    Platform_DebugLog("Cooked:\n");
                    Platform_DebugLog("\tLS: (%f, %f) - %c\n", gamepad->data.left[0], gamepad->data.left[1],
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_LS] & 1));
                    Platform_DebugLog("\tRS: (%f, %f) - %c\n", gamepad->data.right[0], gamepad->data.right[1],
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_RS] & 1));
                    Platform_DebugLog("\tA, B: %c, %c\n",
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_A] & 1),
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_B] & 1));
                    Platform_DebugLog("\tX, Y: %c, %c\n",
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_X] & 1),
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_Y] & 1));
                    Platform_DebugLog("\tLB, RB: %c, %c\n",
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_LB] & 1),
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_RB] & 1));
                    Platform_DebugLog("\tLT, RT: %f, %f\n", gamepad->data.lt, gamepad->data.rt);
                    Platform_DebugLog("\tBack, Start: %c, %c\n",
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_Back] & 1),
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_Start] & 1));
                    Platform_DebugLog("\tPOV right, up, left, down: %c%c%c%c\n",
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_Right] & 1),
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_Up] & 1),
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_Left] & 1),
                                      '0' + (gamepad->data.buttons[Input_GamepadButton_Down] & 1));
                }
#endif
                
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
    if (global_gamepad_free_count <= 0)
        return DIENUM_STOP;
    
    const GUID* guid = &instance->guidInstance;
    if (XInputGetState && IsXInputDevice(&instance->guidProduct))
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
    gamepad->api = GamepadAPI_DirectInput;
    gamepad->dinput.guid = *guid;
    gamepad->dinput.map = &global_gamepadmap_default;
    
    SortGamepadObjects(gamepad);
    
    int32 index_of_map_being_used = -1;
    
    // NOTE(ljre): Look for a mapping for this gamepad
    {
        char guid_str[64];
        char name[256];
        
        // NOTE(ljre): GLFW code with modifications!
        // https://github.com/glfw/glfw/blob/6876cf8d7e0e70dc3e4d7b0224d08312c9f78099/src/win32_joystick.c#L452
        
        // Generate a joystick GUID that matches the SDL 2.0.5+ one
        if (!WideCharToMultiByte(CP_UTF8, 0, instance->tszInstanceName, -1, name, sizeof name, NULL, NULL))
            goto _just_use_default_map;
        
        if (memcmp(&instance->guidProduct.Data4[2], "PIDVID", 6) == 0)
        {
            snprintf(guid_str, sizeof guid_str, "03000000%02x%02x0000%02x%02x000000000000",
                     (uint8) instance->guidProduct.Data1,
                     (uint8) (instance->guidProduct.Data1 >> 8),
                     (uint8) (instance->guidProduct.Data1 >> 16),
                     (uint8) (instance->guidProduct.Data1 >> 24));
        }
        else
        {
            snprintf(guid_str, sizeof guid_str, "05000000%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x00",
                     name[0], name[1], name[2], name[3],
                     name[4], name[5], name[6], name[7],
                     name[8], name[9], name[10]);
        }
        
        //Platform_DebugLog("SDL's Device GUID: %s\n", guid_str);
        
        for (int32 i = 0; i < ArrayLength(global_gamepadmap_database); ++i) {
            const GamepadMappings* map = &global_gamepadmap_database[i];
            
            if (memcmp(guid_str, map->guid, ArrayLength(map->guid)) == 0)
            {
                gamepad->dinput.map = map;
                index_of_map_being_used = i;
                break;
            }
        }
    }
    
    _just_use_default_map:;
    
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
    Trace("Win32_CheckForGamepads");
    
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
                global_gamepads[index].api == GamepadAPI_XInput &&
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
            gamepad->api = GamepadAPI_XInput;
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
    Trace("Win32_InitInput");
    
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

internal inline void
Win32_UpdateInputPre(void)
{
    // NOTE(ljre): Update Keyboard
    for (int32 i = 0; i < ArrayLength(global_keyboard_keys); ++i)
    {
        bool8 key = global_keyboard_keys[i];
        key = (bool8)(key << 1) | (key & 1);
        global_keyboard_keys[i] = key;
    }
}

internal inline void
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

API void
Input_GetMouse(Input_Mouse* out_mouse)
{
    *out_mouse = global_mouse;
}

API bool32
Input_GetGamepad(int32 index, Input_Gamepad* out_gamepad)
{
    if (index < 0 || index >= ArrayLength(global_gamepads))
        return false;
    
    Win32_Gamepad* gamepad = &global_gamepads[index];
    if (!gamepad->connected)
        return false;
    
    *out_gamepad = gamepad->data;
    return true;
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
            // NOTE(ljre): You can't. At least afaik.
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
            // NOTE(ljre): unreachable
        } break;
    }
}

API int32
Input_ConnectedGamepads(Input_Gamepad* gamepads, int32 max_count)
{
    // NOTE(ljre): Return how many gamepads were written to the array
    if (max_count > 0)
    {
        int32 count = 0;
        
        for (int32 i = 0; i < ArrayLength(global_gamepads) && count < max_count; ++i)
        {
            if (global_gamepads[i].connected)
                gamepads[count++] = global_gamepads[i].data;
        }
        
        return count;
    }
    else // NOTE(ljre): Just return how many gamepads are connected
    {
        return (int32)ArrayLength(global_gamepads) - global_gamepad_free_count;
    }
}

API int32
Input_ConnectedGamepadsIndices(int32* indices, int32 max_count)
{
    if (max_count > 0)
    {
        int32 count = 0;
        
        for (int32 i = 0; i < ArrayLength(global_gamepads) && count < max_count; ++i)
        {
            if (global_gamepads[i].connected)
                indices[count++] = i;
        }
        
        return count;
    }
    else
    {
        return (int32)ArrayLength(global_gamepads) - global_gamepad_free_count;
    }
}
