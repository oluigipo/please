#ifndef INTERNAL_2DSCENE_H
#define INTERNAL_2DSCENE_H

#define MAX_2DSCENE_LAYERS 4
#define MAX_2DSCENE_LIGHTS 32

enum Render_2DLayerFlags
{
    Render_2DLayerFlags_Tilemap = 1,
    Render_2DLayerFlags_Sprites = 2,
    Render_2DLayerFlags_Lights = 4,
} typedef Render_2DLayerFlags;

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

struct Render_2DLayer_Light
{
    vec2 pos;
    vec3 color;
    
    float32 constant;
    float32 linear;
    float32 quadratic;
} typedef Render_2DLayer_Light;

struct Render_2DLayer
{
    Render_2DLayerFlags flags;
    Asset_Texture* texture;
    
    struct {
        int32 count;
        Render_2DLayer_Light* data;
    } light;
    
    struct {
        vec2 pos;
        vec2 scale;
        int32 width, height;
        float32 cell_width, cell_height;
        
        uint16* data;
    } tilemap;
    
    struct {
        int32 count;
        Render_2DLayer_Sprite* data;
    } sprite;
} typedef Render_2DLayer;

struct Render_2DScene
{
    Render_Camera camera;
    
    int32 layer_count;
    Render_2DLayer layers[MAX_2DSCENE_LAYERS];
    
    uint32 data_vbo;
    int32 data_vbo_size;
    uint32 data_vao;
} typedef Render_2DScene;

#endif //INTERNAL_2DSCENE_H
