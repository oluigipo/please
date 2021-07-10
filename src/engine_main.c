struct alignas(16) StackAllocatorHeader typedef StackAllocatorHeader;
struct StackAllocatorHeader
{
    StackAllocatorHeader* previous;
    uintsize size; // NOTE(ljre): This includes the size of the header itself
};

//~ Globals
internal void* global_stack_memory;
internal uintsize global_stack_commited_size;
internal uintsize global_stack_reserved_size;
internal StackAllocatorHeader* global_stack_header;
internal const uintsize global_stack_commiting_pages = Megabytes(1);

//~ API
API void*
Engine_PushMemory(uintsize size)
{
    uint8* result = NULL;
    size = AlignUp(size, 15u) + sizeof(StackAllocatorHeader);
    
    if (global_stack_header)
    {
        result = (uint8*)global_stack_header + global_stack_header->size;
    }
    else
    {
        result = global_stack_memory;
    }
    
    uintsize total_desired_size = (uintsize)(result - (uintsize)global_stack_memory + size);
    
    if (total_desired_size > global_stack_reserved_size)
    {
        result = Platform_HeapAlloc(size);
    }
    else
    {
        while (total_desired_size > global_stack_commited_size)
        {
            Platform_VirtualCommit((uint8*)global_stack_memory + global_stack_commited_size, global_stack_commiting_pages);
            global_stack_commited_size += global_stack_commiting_pages;
        }
        
        memset(result, 0, size);
        
        StackAllocatorHeader* header = (void*)result;
        header->size = size;
        header->previous = global_stack_header;
        global_stack_header = header;
        
        result += sizeof(*header);
    }
    
    return result;
}

API void
Engine_PopMemory(void* ptr)
{
    if (ptr < global_stack_memory || (uint8*)ptr >= (uint8*)global_stack_memory + global_stack_reserved_size)
    {
        Platform_HeapFree(ptr);
    }
    else
    {
        StackAllocatorHeader* header = ptr;
        header -= 1;
        
        if (global_stack_header == header)
        {
            global_stack_header = header->previous;
        }
        else
        {
            StackAllocatorHeader* next_header = (void*)((uint8*)header + header->size);
            next_header->previous = header->previous;
            
            if (header->previous)
            {
                header->previous->size += header->size;
            }
        }
    }
}

//~ Entry Point
internal void
DrawGamepadLayout(const Input_Gamepad* gamepad, float32 width, float32 height)
{
    vec4 black = { 0.1f, 0.1f, 0.1f, 1.0f };
    vec4 colors[2] = {
        { 0.3f, 0.3f, 0.3f, 1.0f }, // Enabled
        { 0.9f, 0.9f, 0.9f, 1.0f }, // Disabled
    };
    
    // NOTE(ljre): Draw Axes
    {
        Render_DrawRectangle(black,
                             (vec3) { width / 2.0f - 100.0f, height / 2.0f + 100.0f },
                             (vec3) { 50.0f, 50.0f });
        
        Render_DrawRectangle(colors[Input_IsDown(*gamepad, Input_GamepadButton_LS)],
                             (vec3) { width / 2.0f - 100.0f + gamepad->left[0] * 20.0f,
                                 height / 2.0f + 100.0f + gamepad->left[1] * 20.0f },
                             (vec3) { 20.0f, 20.0f });
        
        Render_DrawRectangle(black,
                             (vec3) { width / 2.0f + 100.0f, height / 2.0f + 100.0f },
                             (vec3) { 50.0f, 50.0f });
        
        Render_DrawRectangle(colors[Input_IsDown(*gamepad, Input_GamepadButton_RS)],
                             (vec3) { width / 2.0f + 100.0f + gamepad->right[0] * 20.0f, height / 2.0f + 100.0f + gamepad->right[1] * 20.0f },
                             (vec3) { 20.0f, 20.0f });
    }
    
    // NOTE(ljre): Draw Buttons
    {
        struct Pair
        {
            vec3 pos;
            vec3 size;
        } buttons[] = {
            [Input_GamepadButton_Y] = { { width * 0.84f, height * 0.44f }, { 30.0f, 30.0f } },
            [Input_GamepadButton_B] = { { width * 0.90f, height * 0.50f }, { 30.0f, 30.0f } },
            [Input_GamepadButton_A] = { { width * 0.84f, height * 0.56f }, { 30.0f, 30.0f } },
            [Input_GamepadButton_X] = { { width * 0.78f, height * 0.50f }, { 30.0f, 30.0f } },
            
            [Input_GamepadButton_LB] = { { width * 0.20f, height * 0.30f }, { 60.0f, 20.0f } },
            [Input_GamepadButton_RB] = { { width * 0.80f, height * 0.30f }, { 60.0f, 20.0f } },
            
            [Input_GamepadButton_Up]    = { { width * 0.16f, height * 0.44f }, { 30.0f, 30.0f } },
            [Input_GamepadButton_Right] = { { width * 0.22f, height * 0.50f }, { 30.0f, 30.0f } },
            [Input_GamepadButton_Down]  = { { width * 0.16f, height * 0.56f }, { 30.0f, 30.0f } },
            [Input_GamepadButton_Left]  = { { width * 0.10f, height * 0.50f }, { 30.0f, 30.0f } },
            
            [Input_GamepadButton_Back]  = { { width * 0.45f, height * 0.48f }, { 25.0f, 15.0f } },
            [Input_GamepadButton_Start] = { { width * 0.55f, height * 0.48f }, { 25.0f, 15.0f } },
        };
        
        for (int32 i = 0; i < ArrayLength(buttons); ++i)
        {
            Render_DrawRectangle(colors[Input_IsDown(*gamepad, i)], buttons[i].pos, buttons[i].size);
        }
    }
    
    // NOTE(ljre): Draw Triggers
    {
        vec4 color;
        
        glm_vec4_lerp(colors[0], colors[1], gamepad->lt, color);
        Render_DrawRectangle(color, (vec3) { width * 0.20f, height * 0.23f }, (vec3) { 60.0f, 30.0f });
        
        glm_vec4_lerp(colors[0], colors[1], gamepad->rt, color);
        Render_DrawRectangle(color, (vec3) { width * 0.80f, height * 0.23f }, (vec3) { 60.0f, 30.0f });
    }
}

API int32
Engine_Main(int32 argc, char** argv)
{
    // NOTE(ljre): Init Global Stack Allocator
    {
        global_stack_reserved_size = Megabytes(128);
        global_stack_memory = Platform_VirtualAlloc(global_stack_reserved_size);
        global_stack_commited_size = global_stack_commiting_pages;
        Platform_VirtualCommit(global_stack_memory, global_stack_commited_size);
        global_stack_header = NULL;
    }
    
    // NOTE(ljre): Window width & height
    float32 width = 600.0f;
    float32 height = 600.0f;
    
    if (!Platform_CreateWindow((int32)width, (int32)height, Str("Title"), GraphicsAPI_OpenGL, &global_graphics))
        Platform_ExitWithErrorMessage(Str("Your computer doesn't seem to support OpenGL 3.3.\nFailed to open."));
    
    Engine_InitRandom();
    Engine_InitRender();
    Discord_Init(778719957956689922ll);
    
    // NOTE(ljre): Load model in custom format 'simple obj (.sobj)'
    Render_3DModel model;
    if (!Render_Load3DModelFromFile(Str("./assets/cube.sobj"), &model))
    {
        Platform_MessageBox(Str("Error"), Str("Could not load cube :("));
    }
    
    // NOTE(ljre): Load Audio
#if 0
    Audio_SoundBuffer sound_music;
    Audio_SoundBuffer sound_music2;
    
    if (!Audio_LoadFile(Str("music.ogg"), &sound_music) ||
        Audio_LoadFile(Str("music2.ogg"), &sound_music2))
    {
        Platform_MessageBox(Str("Warning!"), Str("Could not load 'music.ogg' file. You can replace it and restart the application."));
    }
    else
    {
        Audio_Play(&sound_music, true, 0.3, 2.0);
        Audio_Play(&sound_music2, false, 2.5, 1.0);
    }
#endif
    
    Trace("Audio_LoadFile");
    Audio_SoundBuffer sound_music3 = { 0 };
    Audio_LoadFile(Str("music3.ogg"), &sound_music3);
    
    // NOTE(ljre): Load Font
    Trace("Render_LoadFontFromFile");
    Render_Font font;
    bool32 font_loaded = Render_LoadFontFromFile(Str("arial.ttf"), &font);
    
    int32 controller_index = 0;
    Render_Camera camera = {
        .pos = { 0.0f, 0.0f, 0.0f },
        .dir = { 0.0f, 0.0f, -1.0f },
        .up = { 0.0f, 1.0f, 0.0f },
    };
    
    Trace("Game Loop");
    while (!Platform_WindowShouldClose())
    {
        if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape))
            break;
        
        if (Input_KeyboardIsPressed(Input_KeyboardKey_Left))
            controller_index = (controller_index - 1) & 15;
        else if (Input_KeyboardIsPressed(Input_KeyboardKey_Right))
            controller_index = (controller_index + 1) & 15;
        
        Input_Gamepad gamepad;
        bool32 is_connected = Input_GetGamepad(controller_index, &gamepad);
        
        Render_ClearBackground(is_connected ? 0xFF7F00FF : 0xFF110011);
        
        //~ NOTE(ljre): 3D World
        Render_Begin3D(&camera);
        {
            //- NOTE(ljre): Draw Model
            mat4 where = GLM_MAT4_IDENTITY;
            glm_translate(where, (vec3) { -0.25f, -0.25f, -0.5f });
            glm_rotate(where, (float32)Platform_GetTime(), (vec3) { 0.0f, 1.0f, 0.0f });
            glm_scale(where, (vec3) { 0.25f, 0.25f, 0.25f });
            glm_translate(where, (vec3) { -0.5f, -0.5f, -0.5f });
            
            Render_Draw3DModel(&model, where, 0xFFFFFFFF);
        }
        
        //~ NOTE(ljre): 2D User Interface
        Render_Begin2D(NULL);
        {
            if (is_connected)
            {
                DrawGamepadLayout(&gamepad, width, height);
            }
            
            //- NOTE(ljre): Draw Controller Index
            if (font_loaded) {
                char buf[128];
                snprintf(buf, sizeof buf, "Controller Index: %i", controller_index);
                Render_DrawText(&font, Str(buf), (vec3) { 10.0f, 10.0f }, 32.0f, 0xFFFFFFFF);
            }
            
            //- NOTE(ljre): Audio Testing
            if (sound_music3.samples)
            {
                if (font_loaded) {
                    Render_DrawText(&font, Str("Press J, K, or L!"), (vec3) { 30.0f, 500.0f }, 24.0f, 0xFFFFFFFF);
                }
                
                if (Input_KeyboardIsPressed('L'))
                    Audio_Play(&sound_music3, false, 0.5, 1.5);
                if (Input_KeyboardIsPressed('K'))
                    Audio_Play(&sound_music3, false, 0.5, 1.0);
                if (Input_KeyboardIsPressed('J'))
                    Audio_Play(&sound_music3, false, 0.5, 0.75);
            }
        }
        
        //~ NOTE(ljre): Finish Frame
        Engine_UpdateAudio();
        Discord_Update();
        
#ifdef DEBUG
        if (global_stack_header != NULL)
        {
            Platform_DebugMessageBox("Memory Leak!!!!\nFirst Header - Allocation Size: %zu", global_stack_header->size);
        }
#endif
        // NOTE(ljre): Free all the memory
        global_stack_header = NULL;
        
        Platform_FinishFrame();
        Platform_PollEvents();
    }
    
    Discord_Deinit();
    Engine_DeinitRender();
    
    return 0;
}
