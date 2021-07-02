internal bool32
Win32_CreateDirect3DWindow(int32 width, int32 height, const wchar_t* title)
{
    Trace("Win32_CreateDirect3DWindow");
    
    DWORD style = (WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    HWND window = CreateWindowExW(0, global_class_name, title, style,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  width, height,
                                  NULL, NULL, global_instance, NULL);
    
    if (!window)
        return false;
    
    HDC hdc = GetDC(window);
    
    // TODO
    
    ShowWindow(window, SW_SHOWDEFAULT);
    
    global_window = window;
    global_hdc = hdc;
    
    global_graphics_context.api = GraphicsAPI_Direct3D;
    
    return true;
}
