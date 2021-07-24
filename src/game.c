#include "internal.h"

struct Game_GlobalData
{
    Asset_Font font;
} typedef Game_GlobalData;

#include "game_3ddemo.c"
#include "game_2ddemo.c"

struct Game_MenuButton
{
    vec3 position;
    vec3 size;
    String text;
} typedef Game_MenuButton;

internal bool32
IsMouseOverButton(Input_Mouse* mouse, Game_MenuButton* button)
{
    return ((mouse->pos[0] >= button->position[0] && mouse->pos[0] <= button->position[0] + button->size[0]) &&
            (mouse->pos[1] >= button->position[1] && mouse->pos[1] <= button->position[1] + button->size[1]));
}

internal void
DrawMenuButton(Game_GlobalData* global, Game_MenuButton* button, Input_Mouse* mouse)
{
    vec4 highlight_none = { 0.1f, 0.1f, 0.1f, 1.0f };
    vec4 highlight_over = { 0.2f, 0.2f, 0.2f, 1.0f };
    vec4 highlight_pressed = { 0.4f, 0.4f, 0.4f, 1.0f };
    
    float32* color = highlight_none;
    if (IsMouseOverButton(mouse, button))
    {
        color = highlight_over;
        
        if (Input_IsDown(*mouse, Input_MouseButton_Left))
        {
            color = highlight_pressed;
        }
    }
    
    Render_DrawRectangle(color, button->position, button->size, (vec3) { 0 });
    
    vec3 pos = {
        button->position[0] + button->size[0] / 2.0f,
        button->position[1] + button->size[1] / 2.0f,
    };
    
    vec3 alignment = { -0.5f, -0.5f, 0.0f };
    
    Render_DrawText(&global->font, button->text, pos, 24.0f, (vec4) { 1.0f, 1.0f, 1.0f, 1.0f }, alignment);
}

API void*
Game_MainScene(void** shared_data)
{
    Scene next_scene = NULL;
    
    *shared_data = Engine_PushMemory(sizeof(Game_GlobalData));
    Game_GlobalData* global = *shared_data;
    
    // NOTE(ljre): Load font
    bool32 font_loaded = Render_LoadFontFromFile(Str("./assets/FalstinRegular-XOr2.ttf"), &global->font);
    if (!font_loaded)
    {
        Platform_ExitWithErrorMessage(Str("Could not load the default font :("));
    }
    
    // NOTE(ljre): Menu Buttons
    Game_MenuButton button_3dscene = {
        .position = { 100.0f, 200.0f },
        .size = { 200.0f, 50.0f },
        .text = StrI("Run 3D Scene"),
    };
    
    Game_MenuButton button_2dscene = {
        .position = { 100.0f, 300.0f },
        .size = { 200.0f, 50.0f },
        .text = StrI("Run 2D Scene"),
    };
    
    Game_MenuButton button_quit = {
        .position = { 100.0f, 400.0f },
        .size = { 200.0f, 50.0f },
        .text = StrI("Quit"),
    };
    
    // NOTE(ljre): Game Loop
    while (!Platform_WindowShouldClose())
    {
        Trace("Game Loop");
        void* memory_state = Engine_PushMemoryState();
        
        Input_Mouse mouse;
        Input_GetMouse(&mouse);
        
        //~ NOTE(ljre): Update
        {
            if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape))
            {
                next_scene = NULL;
                break;
            }
            
            if (Input_IsReleased(mouse, Input_MouseButton_Left))
            {
                if (IsMouseOverButton(&mouse, &button_3dscene))
                {
                    next_scene = Game_3DDemoScene;
                }
                //else if (IsMouseOverButton(&mouse, &button_2dscene))
                //{
                //next_scene = Game_2DDemoScene;
                //}
                else if (IsMouseOverButton(&mouse, &button_quit))
                {
                    next_scene = NULL;
                    break;
                }
            }
        }
        
        //~ NOTE(ljre): Render Menu
        {
            Render_Begin2D();
            
            Render_ClearBackground(0.0f, 0.0f, 0.0f, 1.0f);
            Render_DrawText(&global->font, Str("Welcome!\nThose are buttons"), (vec3) { 10.0f, 10.0f }, 32.0f, (vec4) { 1.0f, 1.0f, 1.0f, 1.0f }, (vec3) { 0 });
            
            DrawMenuButton(global, &button_3dscene, &mouse);
            //DrawMenuButton(global, &button_2dscene, &mouse);
            DrawMenuButton(global, &button_quit, &mouse);
        }
        
        //~ NOTE(ljre): Finish Frame
        Engine_PopMemoryState(memory_state);
        Engine_FinishFrame();
        
        if (next_scene != NULL)
            break;
    }
    
    return next_scene;
}
