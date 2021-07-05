// TODO


//~ API
API bool32
Input_KeyboardIsPressed(int32 key)
{
    return false;
}

API bool32
Input_KeyboardIsDown(int32 key)
{
    return false;
}

API bool32
Input_KeyboardIsReleased(int32 key)
{
    return false;
}

API bool32
Input_KeyboardIsUp(int32 key)
{
    return false;
}

API void
Input_GetMouse(Input_Mouse* out_mouse)
{
    *out_mouse = (Input_Mouse) { 0 };
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
