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
    if (!Render_LoadTextureFromFile(Str("./assets/tileset.png"), &tileset_texture))
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
    scene.camera.zoom = 2.0f;
    scene.camera.angle = 0.0f;
    
    //- NOTE(ljre): Tilemap
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
    
    {
        Render_2DLayer* layer = &scene.layers[scene.layer_count++];
        
        layer->flags = Render_2DLayerFlags_Tilemap;
        layer->texture = &tileset_texture;
        layer->tilemap.pos[0] = 0.0f;
        layer->tilemap.pos[1] = -50.0f;
        layer->tilemap.scale[0] = 1.0f;
        layer->tilemap.scale[1] = 1.0f;
        layer->tilemap.width = 8;
        layer->tilemap.height = 8;
        layer->tilemap.cell_width = 32.0f;
        layer->tilemap.cell_height = 32.0f;
        layer->tilemap.data = map;
    }
    
    //- NOTE(ljre): Sprites
    Render_2DLayer_Sprite sprites[30];
    
    {
        Render_2DLayer* layer = &scene.layers[scene.layer_count++];
        
        layer->flags = Render_2DLayerFlags_Sprites;
        layer->texture = &sprites_texture;
        layer->sprite.count = ArrayLength(sprites);
        layer->sprite.data = sprites;
        
        for (int32 i = 0; i < layer->sprite.count; ++i)
        {
            Render_2DLayer_Sprite* sprite = &layer->sprite.data[i];
            
            sprite->pos[0] = Engine_RandomF32Range(-500.0f, 500.0f);
            sprite->pos[1] = Engine_RandomF32Range(-500.0f, 500.0f);
            sprite->scale[0] = 1.0f;
            sprite->scale[1] = 1.0f;
            sprite->angle = Engine_RandomF32Range(0.0f, PI32 * 2.0f);
            sprite->frame = 0;
            sprite->color[0] = 1.0f;
            sprite->color[1] = 1.0f;
            sprite->color[2] = 1.0f;
            sprite->color[3] = 1.0f;
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
