#ifndef INTERNAL_3DMANAGER_H
#define INTERNAL_3DMANAGER_H

#define MAX_3DMANAGER_ENTITIES 256
#define MAX_3DMANAGER_POINT_LIGHTS 8

typedef struct Render_3DEntity Render_3DEntity;
enum Render_3DEntityKind
{
    Render_3DEntityKind_PointLight,
    Render_3DEntityKind_Model,
} typedef Render_3DEntityKind;

struct Render_3DEntity_Model
{
    Render_3DEntity* entity;
    Asset_3DModel* asset;
    vec4 color;
} typedef Render_3DEntity_Model;

struct Render_3DEntity_PointLight
{
    Render_3DEntity* entity;
    
    float32 constant, linear, quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} typedef Render_3DEntity_PointLight;

struct Render_3DEntity
{
    vec3 position;
    vec3 scale;
    vec3 rotation;
    Render_3DEntityKind kind;
    
    union {
        void* handle;
        Render_3DEntity_PointLight* point_light;
        Render_3DEntity_Model* model;
    };
};

struct Render_3DManager
{
    vec3 dirlight;
    Asset_3DModel* cube_model;
    uint32 shadow_fbo, shadow_depthmap;
    uint32 gbuffer, gbuffer_pos, gbuffer_norm, gbuffer_albedo, gbuffer_depth;
    
    Render_3DEntity entities[MAX_3DMANAGER_ENTITIES];
    Render_3DEntity* free_spaces[MAX_3DMANAGER_ENTITIES];
    
    Render_3DEntity_PointLight point_lights[MAX_3DMANAGER_POINT_LIGHTS];
    Render_3DEntity_Model models[MAX_3DMANAGER_ENTITIES];
    
    int32 entity_count;
    int32 free_space_count;
    int32 point_lights_count;
    int32 model_count;
} typedef Render_3DManager;

#endif //INTERNAL_3DMANAGER_H
