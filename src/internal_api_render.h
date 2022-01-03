#ifndef INTERNAL_API_RENDER_H
#define INTERNAL_API_RENDER_H

struct Render_Camera
{
	vec3 pos;
	
	union
	{
		// NOTE(ljre): 2D Stuff
		struct { vec2 size; float32 zoom; float32 angle; };
		
		// NOTE(ljre): 3D Stuff
		struct { vec3 dir; vec3 up; };
	};
}
typedef Render_Camera;

struct Render_3DPointLight
{
	vec3 position;
	
	float32 constant, linear, quadratic;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
}
typedef Render_3DPointLight;

struct Render_3DModel
{
	mat4 transform;
	Asset_3DModel* asset;
	vec4 color;
}
typedef Render_3DModel;

struct Render_3DScene
{
	Asset_3DModel* light_model;
	vec3 dirlight;
	vec3 dirlight_color;
	
	uint32 shadow_fbo, shadow_depthmap;
	uint32 gbuffer, gbuffer_pos, gbuffer_norm, gbuffer_albedo, gbuffer_depth;
	
	int32 point_light_count;
	int32 model_count;
	
	Render_3DPointLight* point_lights;
	Render_3DModel* models;
}
typedef Render_3DScene;

struct Render_Sprite2D
{
	mat4 transform;
	vec4 texcoords;
	vec4 color;
}
typedef Render_Sprite2D;

struct Render_Layer2D
{
	Asset_Texture* texture;
	
	int32 sprite_count;
	Render_Sprite2D* sprites;
}
typedef Render_Layer2D;

API bool32 Render_LoadFontFromFile(String path, Asset_Font* out_font);
API bool32 Render_Load3DModelFromFile(String path, Asset_3DModel* out);
API bool32 Render_LoadTextureFromFile(String path, Asset_Texture* out_texture);
API bool32 Render_LoadTextureArrayFromFile(String path, Asset_Texture* out_texture, int32 cell_width, int32 cell_height);

API void Render_CalcViewMatrix2D(const Render_Camera* camera, mat4 out_view);
API void Render_CalcViewMatrix3D(const Render_Camera* camera, mat4 out_view, float32 fov, float32 aspect);
API void Render_CalcModelMatrix2D(const vec2 pos, const vec2 scale, float32 angle, mat4 out_view);
API void Render_CalcModelMatrix3D(const vec3 pos, const vec3 scale, const vec3 rot, mat4 out_view);

API void Render_ClearBackground(float32 r, float32 g, float32 b, float32 a);
API void Render_Begin2D(void);
API void Render_DrawRectangle(vec4 color, vec3 pos, vec3 size, vec3 alignment);
API void Render_DrawText(const Asset_Font* font, String text, const vec3 pos, float32 char_height, const vec4 color, const vec3 alignment);

API void Render_Draw3DScene(Render_3DScene* scene, const Render_Camera* camera);
API void Render_ProperlySetup3DScene(Render_3DScene* scene);

API void Render_DrawLayer2D(const Render_Layer2D* layer, const mat4 view);

#endif //INTERNAL_API_RENDER_H
