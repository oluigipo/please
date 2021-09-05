// TODO(ljre): Gamepad Input. Unfortunately I couldn't get my USB gamepad to
//             work inside VirtualBox :(
//
// Resources:
// - https://ourmachinery.com/post/gamepad-implementation-on-linux/
// - https://github.com/MysteriousJ/Joystick-Input-Examples

struct Linux_Gamepad
{
	char path[256];
	int fd;
	//GamepadMappings* map;
	
	
	
}
typedef Linux_Gamepad;

//~ Globals
internal Input_Mouse global_mouse;
internal bool8 global_keyboard_keys[Input_KeyboardKey_Count];
internal bool32 global_already_updated_control;
internal bool32 global_already_updated_shift;

internal const Input_KeyboardKey global_keyboard_key_table[] = {
	[0] = Input_KeyboardKey_Any,
	[1] = Input_KeyboardKey_Control,
	[2] = Input_KeyboardKey_Shift,
	
	[XK_Escape] = Input_KeyboardKey_Escape,
	[XK_Menu] = Input_KeyboardKey_Alt,
	[XK_Tab] = Input_KeyboardKey_Tab,
	[XK_Return] = Input_KeyboardKey_Enter,
	[XK_BackSpace] = Input_KeyboardKey_Backspace,
	[XK_Up] = Input_KeyboardKey_Up,
	[XK_Down] = Input_KeyboardKey_Down,
	[XK_Left] = Input_KeyboardKey_Left,
	[XK_Right] = Input_KeyboardKey_Right,
	[XK_Control_L] = Input_KeyboardKey_LeftControl,
	[XK_Control_R] = Input_KeyboardKey_RightControl,
	[XK_Shift_L] = Input_KeyboardKey_LeftShift,
	[XK_Shift_R] = Input_KeyboardKey_RightShift,
	[XK_Prior] = Input_KeyboardKey_PageUp,
	[XK_Next] = Input_KeyboardKey_PageDown,
	[XK_End] = Input_KeyboardKey_End,
	[XK_Home] = Input_KeyboardKey_Home,
	
	[XK_space] = Input_KeyboardKey_Space,
	
	[XK_0] = Input_KeyboardKey_0,
	Input_KeyboardKey_1, Input_KeyboardKey_2, Input_KeyboardKey_3, Input_KeyboardKey_4,
	Input_KeyboardKey_5, Input_KeyboardKey_6, Input_KeyboardKey_7, Input_KeyboardKey_8,
	Input_KeyboardKey_9,
	
	[XK_A] = Input_KeyboardKey_A,
	Input_KeyboardKey_B, Input_KeyboardKey_C, Input_KeyboardKey_D, Input_KeyboardKey_E,
	Input_KeyboardKey_F, Input_KeyboardKey_G, Input_KeyboardKey_H, Input_KeyboardKey_I,
	Input_KeyboardKey_J, Input_KeyboardKey_K, Input_KeyboardKey_L, Input_KeyboardKey_M,
	Input_KeyboardKey_N, Input_KeyboardKey_O, Input_KeyboardKey_P, Input_KeyboardKey_Q,
	Input_KeyboardKey_R, Input_KeyboardKey_S, Input_KeyboardKey_T, Input_KeyboardKey_U,
	Input_KeyboardKey_V, Input_KeyboardKey_W, Input_KeyboardKey_X, Input_KeyboardKey_Y,
	Input_KeyboardKey_Z,
	
	[XK_F1] = Input_KeyboardKey_F1,
	Input_KeyboardKey_F2, Input_KeyboardKey_F3,  Input_KeyboardKey_F4,  Input_KeyboardKey_F4,
	Input_KeyboardKey_F5, Input_KeyboardKey_F6,  Input_KeyboardKey_F7,  Input_KeyboardKey_F8,
	Input_KeyboardKey_F9, Input_KeyboardKey_F10, Input_KeyboardKey_F11, Input_KeyboardKey_F12,
	Input_KeyboardKey_F13,
	
	[XK_KP_0] = Input_KeyboardKey_Numpad0,
	Input_KeyboardKey_Numpad1, Input_KeyboardKey_Numpad2, Input_KeyboardKey_Numpad3, Input_KeyboardKey_Numpad4,
	Input_KeyboardKey_Numpad5, Input_KeyboardKey_Numpad6, Input_KeyboardKey_Numpad7, Input_KeyboardKey_Numpad8,
	Input_KeyboardKey_Numpad9,
};

//~ Functions
internal bool32
EntryNameIsGamepad(const char* restrict entry_name)
{
	const char* const restrict gamepads_suffix = "-event-joystick";
	
	const char* a = entry_name;
	const char* b = gamepads_suffix;
	
	// NOTE(ljre): Checks if string 'a' ends with string 'b'
	while (*a)
	{
		if (*a++ != *b)
			b = gamepads_suffix;
		else
			++b;
	}
	
	return !*b;
}

internal void
CheckForNewGamepads(void)
{
	DIR* directory = opendir("/dev/input/by-id");
	if (!directory)
	{
		Platform_DebugLog("Could not open /dev/input/by-id/\n");
		return;
	}
	
	struct dirent* entry;
	while (entry = readdir(directory), entry)
	{
		if (entry->d_name[0] == '.')
			continue;
		
		Platform_DebugLog("Directory Entry: %s\n", entry->d_name);
		
		if (!EntryNameIsGamepad(entry->d_name))
			continue;
		
		char path[256];
		snprintf(path, sizeof path, "/dev/input/by-id/%s", entry->d_name);
		
		int32 fd = open(path, O_RDONLY);
		
		// TODO(ljre):
	}
	
	closedir(directory);
}

//~ Private API
internal void
Linux_InitInput(void)
{
	// TODO(ljre):
	//CheckForNewGamepads();
}

internal void
Linux_ProcessKeyboardEvent(KeySym key, bool32 pressed, uint32 state)
{
	// NOTE(ljre): convert to capital letters if needed
	if (key >= XK_a && key <= XK_z)
	{
		key = key - XK_a + XK_A;
	}
	
	if (key >= 0 && key < ArrayLength(global_keyboard_key_table))
	{
		Input_KeyboardKey output_key = global_keyboard_key_table[key];
		
		if (output_key)
		{
			global_keyboard_keys[output_key] &= ~1;
			global_keyboard_keys[output_key] |= pressed;
		}
		
		if (output_key == Input_KeyboardKey_LeftControl || output_key == Input_KeyboardKey_RightControl)
		{
			if (!global_already_updated_control)
			{
				global_already_updated_control = true;
				global_keyboard_keys[2] &= ~1;
			}
			
			global_keyboard_keys[1] |= pressed;
		}
		else if (output_key == Input_KeyboardKey_LeftShift || output_key == Input_KeyboardKey_RightShift)
		{
			if (!global_already_updated_shift)
			{
				global_already_updated_shift = true;
				global_keyboard_keys[2] &= ~1;
			}
			
			global_keyboard_keys[2] |= pressed;
		}
	}
}

internal void
Linux_ProcessMouseEvent(uint32 button, bool32 pressed)
{
	static const Input_MouseButton button_table[] = {
		[1] = Input_MouseButton_Left,
		[2] = Input_MouseButton_Middle,
		[3] = Input_MouseButton_Right,
	};
	
	if (button == 4)
	{
		global_mouse.scroll -= 1;
	}
	else if (button == 5)
	{
		global_mouse.scroll += 1;
	}
	else if (button > 0 && button < ArrayLength(button_table))
	{
		global_mouse.buttons[button_table[button]] &=~ 1;
		global_mouse.buttons[button_table[button]] |= (bool8)pressed;
	}
}

internal void
Linux_UpdateInputPre(void)
{
	// NOTE(ljre): Update Keyboard Input
	for (int32 i = 0; i < ArrayLength(global_keyboard_keys); ++i)
	{
		bool8 key = global_keyboard_keys[i];
		key = (bool8)((key << 1) | (key & 1));
		global_keyboard_keys[i] = key;
	}
	
	global_already_updated_control = false;
	global_already_updated_shift = false;
	
	// NOTE(ljre): Update Mouse Input
	glm_vec2_copy(global_mouse.pos, global_mouse.old_pos);
	global_mouse.scroll = 0;
	
	for (int32 i = 0; i < ArrayLength(global_mouse.buttons); ++i)
	{
		bool8 btn = global_mouse.buttons[i];
		btn = (bool8)((btn << 1) | (btn & 1));
		global_mouse.buttons[i] = btn;
	}
}

internal void
Linux_UpdateInputPos(void)
{
	// NOTE(ljre): Update Cursor Position
	{
		Window root, child;
		int32 root_x, root_y;
		int32 mouse_x, mouse_y;
		uint32 mask;
		
		if (XQueryPointer(global_display, global_window, &root, &child, &root_x, &root_y, &mouse_x, &mouse_y, &mask) &&
			mouse_x >= 0 && mouse_x < global_window_width &&
			mouse_y >= 0 && mouse_y < global_window_height)
		{
			global_mouse.pos[0] = (float32)mouse_x;
			global_mouse.pos[1] = (float32)mouse_y;
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
	
	return (global_keyboard_keys[key] & 1) != 0;
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
	return false;
}

API void
Input_SetGamepad(int32 index, float32 vibration)
{
	
}

API int32
Input_ConnectedGamepads(Input_Gamepad* gamepads, int32 max_count)
{
	return 0;
}

API int32
Input_ConnectedGamepadsIndices(int32* indices, int32 max_count)
{
	return 0;
}
