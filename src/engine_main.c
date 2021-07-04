//~ Globals

//~ Functions
API int32
Engine_Main(int32 argc, char** argv)
{
    Random_Init();
    
    float32 width = 600.0f;
    float32 height = 600.0f;
    
    if (!Platform_CreateWindow((int32)width, (int32)height, Str("Title"), GraphicsAPI_OpenGL, &global_graphics))
        Platform_MessageBox(Str("Warning!"), Str("Your computer doesn't seem to support OpenGL 3.3.\nFailed to open."));
    
    Render_Init();
    
    // NOTE(ljre): Load Audio
#if 1
    Audio_SoundBuffer sound_music;
    Audio_SoundBuffer sound_music2;
    
    if (!Audio_LoadFile(Str("music.ogg"), &sound_music))
    {
        Platform_MessageBox(Str("Warning!"), Str("Could not load 'music.ogg' file. You can replace it and restart the application."));
    }
    else
    {
        Audio_Play(&sound_music, true, 0.3, 2.0);
    }
    
    if (!Audio_LoadFile(Str("music2.ogg"), &sound_music2))
    {
        Platform_MessageBox(Str("Warning!"), Str("Could not load 'music2.ogg' file. You can replace it and restart the application."));
    }
    else
    {
        Audio_Play(&sound_music2, false, 2.5, 1.0);
    }
#endif
    
    Audio_SoundBuffer sound_music3;
    Audio_LoadFile(Str("music3.ogg"), &sound_music3);
    
    // NOTE(ljre): Load Font
    Render_Font font;
    Render_LoadFontFromFile(Str("C:/Windows/Fonts/Arial.ttf"), &font);
    
    int32 controller_index = 0;
    
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
            Render_ClearBackground(0xFF7F00FF);
            
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
                
                Render_DrawRectangle(colors[Input_IsDown(gamepad, Input_GamepadButton_LS)],
                                     (vec3) { width / 2.0f - 100.0f + gamepad.left[0] * 20.0f,
                                         height / 2.0f + 100.0f + gamepad.left[1] * 20.0f },
                                     (vec3) { 20.0f, 20.0f });
                
                Render_DrawRectangle(black,
                                     (vec3) { width / 2.0f + 100.0f, height / 2.0f + 100.0f },
                                     (vec3) { 50.0f, 50.0f });
                
                Render_DrawRectangle(colors[Input_IsDown(gamepad, Input_GamepadButton_RS)],
                                     (vec3) { width / 2.0f + 100.0f + gamepad.right[0] * 20.0f, height / 2.0f + 100.0f + gamepad.right[1] * 20.0f },
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
                    Render_DrawRectangle(colors[Input_IsDown(gamepad, i)], buttons[i].pos, buttons[i].size);
                }
            }
            
            // NOTE(ljre): Draw Triggers
            {
                vec4 color;
                
                glm_vec4_lerp(colors[0], colors[1], gamepad.lt, color);
                Render_DrawRectangle(color, (vec3) { width * 0.20f, height * 0.23f }, (vec3) { 60.0f, 30.0f });
                
                glm_vec4_lerp(colors[0], colors[1], gamepad.rt, color);
                Render_DrawRectangle(color, (vec3) { width * 0.80f, height * 0.23f }, (vec3) { 60.0f, 30.0f });
            }
        }
        else
        {
            Render_ClearBackground(0xFF000000);
        }
        
        // NOTE(ljre): Draw Controller Index
        {
            char buf[128];
            snprintf(buf, sizeof buf, "Controller Index: %i", controller_index);
            Render_DrawText(&font, Str(buf), (vec3) { 10.0f, 10.0f }, 32.0f, 0xFFFFFFFF);
        }
        
        if (sound_music3.samples)
        {
            // NOTE(ljre): Draw funny
            {
                Render_DrawText(&font, Str("Press J, K, or L!"), (vec3) { 30.0f, 550.0f }, 24.0f, 0xFFFFFFFF);
            }
            
            if (Input_KeyboardIsPressed('L'))
                Audio_Play(&sound_music3, false, 0.5, 1.5);
            if (Input_KeyboardIsPressed('K'))
                Audio_Play(&sound_music3, false, 0.5, 1.0);
            if (Input_KeyboardIsPressed('J'))
                Audio_Play(&sound_music3, false, 0.5, 0.75);
        }
        
        Engine_UpdateAudio();
        
        Platform_FinishFrame();
        Platform_PollEvents();
    }
    
    return 0;
}
