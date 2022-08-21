#ifndef INTERNAL_API_RENDER_H
#define INTERNAL_API_RENDER_H

//~ Types
struct Render_Camera3D
{
	vec3 pos;
	vec3 dir;
	vec3 up;
}
typedef Render_Camera3D;

struct Render_Data2DInstance
{
	mat4 transform;
	vec4 texcoords; // [left, top, width, height] normalized
	vec4 color; // [r, g, b, a] normalized
}
typedef Render_Data2DInstance;

enum Render_BlendMode
{
	Render_BlendMode_Normal = 0,
	Render_BlendMode_None,
	Render_BlendMode_Add,
	Render_BlendMode_Subtract,
};

struct Render_Camera2D
{
	vec2 pos;
	vec2 size;
	float32 zoom;
	float32 angle;
}
typedef Render_Camera2D;

struct Render_Data2D
{
	const Asset_Texture* texture;
	enum Render_BlendMode blendmode;
	Render_Camera2D camera;
	
	const Render_Data2DInstance* instances;
	uintsize instance_count;
}
typedef Render_Data2D;

//~ Main API
struct Engine_RenderApi
{
	void (*clear_color)(const vec4 color);
	void (*draw_2d)(const Render_Data2D* data);
	
	//- OLD API
	bool (*load_font_from_file)(String path, Asset_Font* out_font);
	bool (*load_3dmodel_from_file)(String path, Asset_3DModel* out_model);
	bool (*load_texture_from_file)(String path, Asset_Texture* out_texture);
	
	void (*clear_background)(float32 r, float32 g, float32 b, float32 a);
	void (*begin)(void);
	void (*draw_rectangle)(vec4 color, vec3 pos, vec3 size, vec3 alignment);
	void (*draw_texture)(const Asset_Texture* texture, const mat4 transform, const mat4 view, const vec4 color);
	void (*draw_text)(const Asset_Font* font, String text, const vec3 pos, float32 char_height, const vec4 color, const vec3 alignment);
	
	void (*draw_3dscene)(Engine_Renderer3DScene* scene, const Render_Camera3D* camera);
}
typedef Engine_RenderApi;

//~ Wrappers
internal inline void
Render_ClearColor(Engine_Data* engine, const vec4 color)
{ engine->render->clear_color(color); }

internal inline void
Render_Draw2D(Engine_Data* engine, const Render_Data2D* data)
{ engine->render->draw_2d(data); }

#endif //INTERNAL_API_RENDER_H
