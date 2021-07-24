#ifndef INTERNAL_2DSCENE_H
#define INTERNAL_2DSCENE_H

#define MAX_2DSCENE_LAYERS 4
#define MAX_2DSCENE_SPRITES 256

enum Render_2DLayerKind
{
    Render_2DLayerKind_Tilemap,
    Render_2DLayerKind_Sprites,
} typedef Render_2DLayerKind;

struct Render_2DLayer_Sprite
{
    vec2 pos;
    vec2 scale;
    vec2 pivot;
    float32 angle;
    
    int32 frame;
    vec4 color;
    QuadI32 texture_quad;
} typedef Render_2DLayer_Sprite;

struct Render_2DLayer
{
    Render_2DLayerKind kind;
    Asset_Texture* texture;
    
    union
    {
        struct {
            vec2 pos;
            int32 width, height;
            
            uint16* data;
        } tilemap;
        
        struct {
            int32 sprite_count;
            Render_2DLayer_Sprite sprites[MAX_2DSCENE_SPRITES];
        };
    };
} typedef Render_2DLayer;

struct Render_2DScene
{
    Render_Camera camera;
    
    int32 layer_count;
    Render_2DLayer layers[MAX_2DSCENE_LAYERS];
    
    uint32 sprite_ubo;
} typedef Render_2DScene;

#endif //INTERNAL_2DSCENE_H
