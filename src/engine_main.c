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

API void*
Engine_PushMemoryState(void)
{
    return global_stack_header;
}

API void Engine_PopMemoryState(void* state)
{
    global_stack_header = state;
}

//~ Entry Point
internal void
DrawGamepadLayout(const Input_Gamepad* gamepad, float32 x, float32 y, float32 width, float32 height)
{
    vec4 black = { 0.1f, 0.1f, 0.1f, 1.0f };
    vec4 colors[2] = {
        { 0.5f, 0.5f, 0.5f, 1.0f }, // Enabled
        { 0.9f, 0.9f, 0.9f, 1.0f }, // Disabled
    };
    
    float32 xscale = width / 600.0f;
    float32 yscale = height / 600.0f;
    
    // NOTE(ljre): Draw Axes
    {
        Render_DrawRectangle(black,
                             (vec3) { x + width / 2.0f - 100.0f * xscale, y + height / 2.0f + 100.0f * yscale },
                             (vec3) { 50.0f * xscale, 50.0f * yscale });
        
        Render_DrawRectangle(colors[Input_IsDown(*gamepad, Input_GamepadButton_LS)],
                             (vec3) { x + width / 2.0f - 100.0f * xscale + gamepad->left[0] * 20.0f * xscale,
                                 y + height / 2.0f + 100.0f * yscale + gamepad->left[1] * 20.0f * yscale },
                             (vec3) { 20.0f * xscale, 20.0f * yscale });
        
        Render_DrawRectangle(black,
                             (vec3) { x + width / 2.0f + 100.0f * xscale, y + height / 2.0f + 100.0f * yscale },
                             (vec3) { 50.0f * xscale, 50.0f * yscale });
        
        Render_DrawRectangle(colors[Input_IsDown(*gamepad, Input_GamepadButton_RS)],
                             (vec3) { x + width / 2.0f + 100.0f * xscale + gamepad->right[0] * 20.0f * xscale, y + height / 2.0f + 100.0f * yscale + gamepad->right[1] * 20.0f * yscale },
                             (vec3) { 20.0f * xscale, 20.0f * yscale });
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
            buttons[i].pos[0] += x;
            buttons[i].pos[1] += y;
            buttons[i].size[0] *= xscale;
            buttons[i].size[1] *= yscale;
            
            Render_DrawRectangle(colors[Input_IsDown(*gamepad, i)], buttons[i].pos, buttons[i].size);
        }
    }
    
    // NOTE(ljre): Draw Triggers
    {
        vec4 color;
        
        glm_vec4_lerp(colors[0], colors[1], gamepad->lt, color);
        Render_DrawRectangle(color,
                             (vec3) { x + width * 0.20f, y + height * 0.23f },
                             (vec3) { 60.0f * xscale, 30.0f * yscale });
        
        glm_vec4_lerp(colors[0], colors[1], gamepad->rt, color);
        Render_DrawRectangle(color,
                             (vec3) { x + width * 0.80f, y + height * 0.23f },
                             (vec3) { 60.0f * xscale, 30.0f * yscale });
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
    float32 width = 1280.0f;
    float32 height = 720.0f;
    
    if (!Platform_CreateWindow((int32)width, (int32)height, Str("Title"), GraphicsAPI_OpenGL, &global_graphics))
        Platform_ExitWithErrorMessage(Str("Your computer doesn't seem to support OpenGL 3.3.\nFailed to open."));
    
    Engine_InitRandom();
    Engine_InitRender();
    Discord_Init(778719957956689922ll);
    
    // NOTE(ljre): Load model in custom format 'simple obj (.sobj)'
    Render_3DModel model;
    if (!Render_Load3DModelFromFile(Str("./assets/cube.glb"), &model))
    {
        Platform_ExitWithErrorMessage(Str("Could not load cube located in 'assets/cube.sobj'. Are you running this program on the build directory or repository directory? Make sure it's on the repository one."));
    }
    
    Render_3DModel tree_model;
    if (!Render_Load3DModelFromFile(Str("./assets/first_tree.glb"), &tree_model))
    {
        Platform_ExitWithErrorMessage(Str(":("));
    }
    
    // NOTE(ljre): Load Audio
#if 1
    Audio_SoundBuffer sound_music;
    
    if (!Audio_LoadFile(Str("music.ogg"), &sound_music))
    {
        Platform_MessageBox(Str("Warning!"), Str("Could not load 'music.ogg' file. You can replace it and restart the application."));
    }
    else
    {
        Audio_Play(&sound_music, true, 0.4, 1.0);
    }
#endif
    
    Audio_SoundBuffer sound_music3 = { 0 };
    Audio_LoadFile(Str("music3.ogg"), &sound_music3);
    
    // NOTE(ljre): Load Font
    Trace("Render_LoadFontFromFile");
    Render_Font font;
    bool32 font_loaded = Render_LoadFontFromFile(Str("c:/windows/fonts/arial.ttf"), &font);
    
    int32 controller_index = 0;
    
    float32 camera_yaw = PI32;
    float32 camera_pitch = 0.0f;
    float32 sensitivity = 0.05f;
    float32 camera_total_speed = 0.075f;
    float32 camera_height = 1.9f;
    float32 moving_time = 0.0f;
    vec2 camera_speed = { 0 };
    
    Render_Camera camera = {
        .pos = { 0.0f, camera_height, 0.0f },
        .dir = { 0.0f, 0.0f, -1.0f },
        .up = { 0.0f, 1.0f, 0.0f },
    };
    
    struct RotatingCube {
        vec3 pos;
        ColorARGB color;
    };
    
    struct RotatingCube rotating_cubes[30];
    for (int32 i = 0; i < ArrayLength(rotating_cubes); ++i)
    {
        rotating_cubes[i].pos[0] = Engine_RandomF32Range(-20.0f, 20.0f);
        rotating_cubes[i].pos[1] = Engine_RandomF32Range(3.0f, 10.0f);
        rotating_cubes[i].pos[2] = Engine_RandomF32Range(-20.0f, 20.0f);
        
        static const ColorARGB colors[] = {
            0xFFFF0000, 0xFFFF3E00, 0xFFFF8100, 0xFFFFC100, 0xFFFFFF00, 0xFFC1FF00,
            0xFF81FF00, 0xFF3EFF00, 0xFF00FF00, 0xFF00FF3E, 0xFF00FF81, 0xFF00FFC1,
            0xFF00FFFF, 0xFF00C1FF, 0xFF0081FF, 0xFF003EFF, 0xFF0000FF, 0xFF3E00FF,
            0xFF8100FF, 0xFFC100FF, 0xFFFF00FF, 0xFFFF00C1, 0xFFFF0081, 0xFFFF003E,
        };
        
        rotating_cubes[i].color = colors[Engine_RandomU64() % ArrayLength(colors)];
    }
    
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
        
        if (is_connected)
        {
            //~ NOTE(ljre): Camera Direction
            if (gamepad.right[0] != 0.0f)
            {
                camera_yaw += gamepad.right[0] * sensitivity;
                camera_yaw = fmodf(camera_yaw, PI32 * 2.0f);
            }
            
            if (gamepad.right[1] != 0.0f)
            {
                camera_pitch += -gamepad.right[1] * sensitivity;
                camera_pitch = glm_clamp(camera_pitch, -PI32 * 0.49f, PI32 * 0.49f);
            }
            
            float32 pitched = cosf(camera_pitch);
            
            camera.dir[0] = cosf(camera_yaw) * pitched;
            camera.dir[1] = sinf(camera_pitch);
            camera.dir[2] = sinf(camera_yaw) * pitched;
            glm_vec3_normalize(camera.dir);
            
            vec3 right;
            glm_vec3_cross((vec3) { 0.0f, 1.0f, 0.0f }, camera.dir, right);
            
            glm_vec3_cross(camera.dir, right, camera.up);
            
            //~ Movement
            vec2 speed;
            
            speed[0] = -gamepad.left[1];
            speed[1] =  gamepad.left[0];
            
            glm_vec2_rotate(speed, camera_yaw, speed);
            glm_vec2_scale(speed, camera_total_speed, speed);
            glm_vec2_lerp(camera_speed, speed, 0.2f, camera_speed);
            
            camera.pos[0] += camera_speed[0];
            camera.pos[2] += camera_speed[1];
            
            if (Input_IsDown(gamepad, Input_GamepadButton_A))
                camera_height += 0.05f;
            else if (Input_IsDown(gamepad, Input_GamepadButton_B))
                camera_height -= 0.05f;
            
            //~ Bump
            float32 d = glm_vec2_distance2(camera_speed, GLM_VEC2_ZERO);
            if (d > 0.0001f)
            {
                moving_time += 0.3f;
            }
            
            camera.pos[1] = camera_height + sinf(moving_time) * 0.04f;
        }
        
        Render_ClearBackground(is_connected ? 0xFF2D81FF : 0xFF110011);
        
        //~ NOTE(ljre): 3D World
        Render_Begin3D(&camera);
        {
            mat4 where;
            
            //- NOTE(ljre): Draw Models
            for (int32 i = 0; i < ArrayLength(rotating_cubes); ++i)
            {
                struct RotatingCube* cube = &rotating_cubes[i];
                
                glm_mat4_identity(where);
                glm_translate(where, (vec3) { cube->pos[0], cube->pos[1] + 0.5f, cube->pos[2] });
                glm_rotate(where, (float32)Platform_GetTime() + (float32)i, (vec3) { 0.0f, 1.0f, 0.0f });
                glm_translate(where, (vec3) { -0.5f, -0.5f, -0.5f });
                Render_Draw3DModel(&model, where, cube->color);
            }
            
            // Ground
            glm_mat4_identity(where);
            glm_translate(where, (vec3) { 0.0f, -1.0f, 0.0f });
            glm_scale(where, (vec3) { 20.0f, 2.0f, 20.0f });
            glm_translate(where, (vec3) { -0.5f, -0.5f, -0.5f });
            Render_Draw3DModel(&model, where, 0xFF008000);
            
            // Tree
            glm_mat4_identity(where);
            Render_Draw3DModel(&tree_model, where, 0xFFFFFFFF);
        }
        
        //~ NOTE(ljre): 2D User Interface
        Render_Begin2D(NULL);
        {
            //- NOTE(ljre): Draw Controller Index
            if (font_loaded) {
                char buf[128];
                snprintf(buf, sizeof buf, "Controller Index: %i", controller_index);
                Render_DrawText(&font, Str(buf), (vec3) { 10.0f, 10.0f }, 32.0f, 0xFFFFFFFF);
            }
            
            if (is_connected)
            {
                DrawGamepadLayout(&gamepad, 0.0f, 0.0f, 300.0f, 300.0f);
            }
            
            //- NOTE(ljre): Audio Testing
            if (sound_music3.samples)
            {
                if (font_loaded) {
                    Render_DrawText(&font, Str("Press X, A, or B!"), (vec3) { 30.0f, 500.0f }, 24.0f, 0xFFFFFFFF);
                }
                
                
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
