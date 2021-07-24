internal void*
Game_2DDemoScene(void** shared_data)
{
    Trace("Game_2DDemoScene");
    
    Game_GlobalData* global = *shared_data;
    
    //~ NOTE(ljre): Assets
    Asset_Texture sprites_texture;
    if (!Render_LoadTextureFromFile(Str("./assets/sprites.png"), &sprites_texture))
    {
        Platform_ExitWithErrorMessage(Str("Could not load sprites image :("));
    }
    
    Asset_Texture tileset_texture;
    if (!Render_LoadTextureArrayFromFile(Str("./assets/tileset.png"), &tileset_texture, 32, 32))
    {
        Platform_ExitWithErrorMessage(Str("Could not load tileset image :("));
    }
    
    //~ NOTE(ljre): Scene Setup
    Render_2DScene scene = { 0 };
    
    scene.camera.pos[0] = 0.0f;
    scene.camera.pos[1] = 0.0f;
    scene.camera.pos[2] = 0.0f;
    scene.camera.size[0] = (float32)Platform_WindowWidth();
    scene.camera.size[1] = (float32)Platform_WindowHeight();
    scene.camera.zoom = 1.0f;
    scene.camera.angle = 0.0f;
    
    //- NOTE(ljre): Tilemap
    {
        Render_2DLayer* layer = &scene.layers[scene.layer_count++];
        
        uint16 map[8*8] = {
            0, 1, 1, 1, 1, 1, 1, 0,
            1, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 1, 0, 0, 1, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 1, 1, 1, 1, 0, 1,
            1, 0, 0, 1, 1, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 1,
            0, 1, 1, 1, 1, 1, 1, 0,
        };
        
        layer->kind = Render_2DLayerKind_Tilemap;
        layer->texture = &tileset_texture;
        layer->tilemap.pos[0] = 0.0f;
        layer->tilemap.pos[1] = 0.0f;
        layer->tilemap.width = 8;
        layer->tilemap.height = 8;
        layer->tilemap.data = map;
    }
    
    //- NOTE(ljre): Sprites
    {
        Render_2DLayer* layer = &scene.layers[scene.layer_count++];
        
        layer->kind = Render_2DLayerKind_Sprites;
        layer->texture = &sprites_texture;
        layer->sprite_count = 30;
        
        for (int32 i = 0; i < layer->sprite_count; ++i)
        {
            Render_2DLayer_Sprite* sprite = &layer->sprites[i];
            
            sprite->pos[0] = Engine_RandomF32Range(-500.0f, 500.0f);
            sprite->pos[1] = Engine_RandomF32Range(-500.0f, 500.0f);
            sprite->scale[0] = 1.0f;
            sprite->scale[1] = 1.0f;
            sprite->angle = Engine_RandomF32Range(0.0f, PI32 * 2.0f);
            sprite->frame = 0;
            sprite->texture_quad.x = 0;
            sprite->texture_quad.y = 0;
            sprite->texture_quad.width = 32;
            sprite->texture_quad.height = 32;
        }
    }
    
    // NOTE(ljre): Game Loop
    while (!Platform_WindowShouldClose())
    {
        Trace("Game Loop");
        void* memory_state = Engine_PushMemoryState();
        
        //~ NOTE(ljre): Update
        {
            if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape))
                break;
        }
        
        //~ NOTE(ljre): Render
        {
            Render_ClearBackground(0.0f, 0.0f, 0.0f, 1.0f);
            Render_Draw2DScene(&scene);
        }
        
        //~ NOTE(ljre): Finish Frame
        Engine_PopMemoryState(memory_state);
        Engine_FinishFrame();
    }
    
    return NULL;
}
