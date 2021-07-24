internal void
ColorToVec4(uint32 color, vec4 out)
{
    out[0] = (float32)(color>>16 & 0xFF) / 255.0f;
    out[1] = (float32)(color>>8 & 0xFF) / 255.0f;
    out[2] = (float32)(color & 0xFF) / 255.0f;
    out[3] = (float32)(color>>24 & 0xFF) / 255.0f;
}

internal void
DrawGamepadLayout(const Input_Gamepad* gamepad, float32 x, float32 y, float32 width, float32 height)
{
    vec3 alignment = { -0.5f, -0.5f };
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
                             (vec3) { 50.0f * xscale, 50.0f * yscale }, alignment);
        
        Render_DrawRectangle(colors[Input_IsDown(*gamepad, Input_GamepadButton_LS)],
                             (vec3) { x + width / 2.0f - 100.0f * xscale + gamepad->left[0] * 20.0f * xscale,
                                 y + height / 2.0f + 100.0f * yscale + gamepad->left[1] * 20.0f * yscale },
                             (vec3) { 20.0f * xscale, 20.0f * yscale }, alignment);
        
        Render_DrawRectangle(black,
                             (vec3) { x + width / 2.0f + 100.0f * xscale, y + height / 2.0f + 100.0f * yscale },
                             (vec3) { 50.0f * xscale, 50.0f * yscale }, alignment);
        
        Render_DrawRectangle(colors[Input_IsDown(*gamepad, Input_GamepadButton_RS)],
                             (vec3) { x + width / 2.0f + 100.0f * xscale + gamepad->right[0] * 20.0f * xscale, y + height / 2.0f + 100.0f * yscale + gamepad->right[1] * 20.0f * yscale },
                             (vec3) { 20.0f * xscale, 20.0f * yscale }, alignment);
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
            
            Render_DrawRectangle(colors[Input_IsDown(*gamepad, i)], buttons[i].pos, buttons[i].size, alignment);
        }
    }
    
    // NOTE(ljre): Draw Triggers
    {
        vec4 color;
        
        glm_vec4_lerp(colors[0], colors[1], gamepad->lt, color);
        Render_DrawRectangle(color,
                             (vec3) { x + width * 0.20f, y + height * 0.23f },
                             (vec3) { 60.0f * xscale, 30.0f * yscale }, alignment);
        
        glm_vec4_lerp(colors[0], colors[1], gamepad->rt, color);
        Render_DrawRectangle(color,
                             (vec3) { x + width * 0.80f, y + height * 0.23f },
                             (vec3) { 60.0f * xscale, 30.0f * yscale }, alignment);
    }
}

internal void*
Game_3DDemoScene(void** shared_data)
{
    Trace("Game_3DDemoScene");
    
    Game_GlobalData* global = *shared_data;
    
    Asset_3DModel model;
    if (!Render_Load3DModelFromFile(Str("./assets/cube.glb"), &model))
    {
        Platform_ExitWithErrorMessage(Str("Could not load cube located in 'assets/cube.sobj'. Are you running this program on the build directory or repository directory? Make sure it's on the repository one."));
    }
    
    Asset_3DModel tree_model;
    if (!Render_Load3DModelFromFile(Str("./assets/first_tree.glb"), &tree_model))
    {
        Platform_ExitWithErrorMessage(Str("Could not load tree model :("));
    }
    
    Asset_3DModel corset_model;
    if (!Render_Load3DModelFromFile(Str("./assets/corset.glb"), &corset_model))
    {
        Platform_ExitWithErrorMessage(Str("Could not load corset model :("));
    }
    
    // NOTE(ljre): Load Audio
    Asset_SoundBuffer sound_music;
    
    if (!Audio_LoadFile(Str("music.ogg"), &sound_music))
    {
        Platform_MessageBox(Str("Warning!"), Str("Could not load 'music.ogg' file. You can replace it and restart the application."));
    }
    else
    {
        Audio_Play(&sound_music, true, 0.4, 1.0);
    }
    
    Asset_SoundBuffer sound_music3 = { 0 };
    Audio_LoadFile(Str("music3.ogg"), &sound_music3);
    
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
    
    //~ NOTE(ljre): Entities
    Render_3DManager manager = { 0 };
    manager.dirlight[0] = 1.0f;
    manager.dirlight[1] = 2.0f; // TODO(ljre): discover why tf this needs to be positive
    manager.dirlight[2] = 0.5f;
    glm_vec3_normalize(manager.dirlight);
    
    manager.cube_model = &model;
    
    //- Ground
    Render_3DEntity* ground = Render_AddTo3DManager(&manager, Render_3DEntityKind_Model);
    
    ground->scale[0] = 20.0f;
    ground->scale[1] = 2.0f;
    ground->scale[2] = 20.0f;
    ground->position[0] = 0.0f;
    ground->position[1] = -2.0f;
    ground->position[2] = 0.0f;
    ground->model->color[0] = 1.0f;
    ground->model->color[1] = 1.0f;
    ground->model->color[2] = 1.0f;
    ground->model->color[3] = 1.0f;
    ground->model->asset = &model;
    
    //- Tree
    Render_3DEntity* tree = Render_AddTo3DManager(&manager, Render_3DEntityKind_Model);
    
    ColorToVec4(0xFFFFFFFF, tree->model->color);
    tree->position[1] += 0.1f;
    tree->model->asset = &tree_model;
    
    //- Corset
    for (int32 i = 0; i < 10; ++i) {
        Render_3DEntity* corset = Render_AddTo3DManager(&manager, Render_3DEntityKind_Model);
        
        ColorToVec4(0xFFFFFFFF, corset->model->color);
        corset->position[0] = Engine_RandomF32Range(-5.0f, 5.0f);
        corset->position[1] = 0.1f;
        corset->position[2] = Engine_RandomF32Range(-5.0f, 5.0f);
        corset->model->asset = &corset_model;
    }
    
    //- Rotating Cubes
    Render_3DEntity* rotating_cubes[30];
    for (int32 i = 0; i < ArrayLength(rotating_cubes); ++i)
    {
        rotating_cubes[i] = Render_AddTo3DManager(&manager, Render_3DEntityKind_Model);
        
        rotating_cubes[i]->position[0] = Engine_RandomF32Range(-20.0f, 20.0f);
        rotating_cubes[i]->position[1] = Engine_RandomF32Range(3.0f, 10.0f);
        rotating_cubes[i]->position[2] = Engine_RandomF32Range(-20.0f, 20.0f);
        rotating_cubes[i]->model->asset = &model;
        
        static const uint32 colors[] = {
            0xFFFF0000, 0xFFFF3E00, 0xFFFF8100, 0xFFFFC100, 0xFFFFFF00, 0xFFC1FF00,
            0xFF81FF00, 0xFF3EFF00, 0xFF00FF00, 0xFF00FF3E, 0xFF00FF81, 0xFF00FFC1,
            0xFF00FFFF, 0xFF00C1FF, 0xFF0081FF, 0xFF003EFF, 0xFF0000FF, 0xFF3E00FF,
            0xFF8100FF, 0xFFC100FF, 0xFFFF00FF, 0xFFFF00C1, 0xFFFF0081, 0xFFFF003E,
        };
        
        ColorToVec4(colors[Engine_RandomU64() % ArrayLength(colors)], rotating_cubes[i]->model->color);
    }
    
    //- Lights
    Render_3DEntity* light1 = Render_AddTo3DManager(&manager, Render_3DEntityKind_PointLight);
    Render_3DEntity* light2 = Render_AddTo3DManager(&manager, Render_3DEntityKind_PointLight);
    
    light1->position[0] = 1.0f;
    light1->position[1] = 2.0f;
    light1->position[2] = 5.0f;
    light1->scale[0] = 0.25f;
    light1->scale[1] = 0.25f;
    light1->scale[2] = 0.25f;
    light1->point_light->ambient[0] = 0.2f;
    light1->point_light->ambient[1] = 0.0f;
    light1->point_light->ambient[2] = 0.0f;
    light1->point_light->diffuse[0] = 0.8f;
    light1->point_light->diffuse[1] = 0.0f;
    light1->point_light->diffuse[2] = 0.0f;
    light1->point_light->specular[0] = 1.0f;
    light1->point_light->specular[1] = 0.0f;
    light1->point_light->specular[2] = 0.0f;
    light1->point_light->constant = 1.0f;
    light1->point_light->linear = 0.09f;
    light1->point_light->quadratic = 0.032f;
    
    light2->position[0] = 1.0f;
    light2->position[1] = 2.0f;
    light2->position[2] = -5.0f;
    light2->scale[0] = 0.25f;
    light2->scale[1] = 0.25f;
    light2->scale[2] = 0.25f;
    light2->point_light->ambient[0] = 0.0f;
    light2->point_light->ambient[1] = 0.0f;
    light2->point_light->ambient[2] = 1.0f;
    light2->point_light->diffuse[0] = 0.0f;
    light2->point_light->diffuse[1] = 0.0f;
    light2->point_light->diffuse[2] = 1.0f;
    light2->point_light->specular[0] = 0.0f;
    light2->point_light->specular[1] = 0.0f;
    light2->point_light->specular[2] = 0.8f;
    light2->point_light->constant = 1.0f;
    light2->point_light->linear = 0.09f;
    light2->point_light->quadratic = 0.032f;
    
    for (void* memory_state = Engine_PushMemoryState();
         !Platform_WindowShouldClose();
         Engine_PopMemoryState(memory_state))
    {
        Trace("Game Loop");
        
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
        
        if (is_connected)
            Render_ClearBackground(0.1f, 0.15f, 0.3f, 1.0f);
        else
            Render_ClearBackground(0.0f, 0.0f, 0.0f, 1.0f);
        
        //~ NOTE(ljre): 3D World
        float32 t = (float32)Platform_GetTime();
        
        for (int32 i = 0; i < ArrayLength(rotating_cubes); ++i)
        {
            rotating_cubes[i]->rotation[1] = t + (float32)i;
        }
        
        Render_Draw3DManager(&manager, &camera);
        
        //~ NOTE(ljre): 2D User Interface
        Render_Begin2D();
        
        //- NOTE(ljre): Draw Controller Index
        const vec4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
        
        char buf[128];
        snprintf(buf, sizeof buf, "Controller Index: %i", controller_index);
        Render_DrawText(&global->font, Str(buf), (vec3) { 10.0f, 10.0f }, 32.0f, white, (vec3) { 0 });
        
        Render_DrawText(&global->font, Str("Press X, A, or B!"), (vec3) { 30.0f, 500.0f }, 24.0f, white, (vec3) { 0 });
        
        if (is_connected)
        {
            DrawGamepadLayout(&gamepad, 0.0f, 0.0f, 300.0f, 300.0f);
        }
        
        //~ NOTE(ljre): Finish Frame
        Engine_FinishFrame();
    }
    
    return NULL;
}

