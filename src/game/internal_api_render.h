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

#ifndef INTERNAL_TEST_OPENGL_NEWREN

struct Render_Data2D
{
	const Asset_Texture* texture;
	enum Render_BlendMode blendmode;
	Render_Camera2D camera;
	
	const Render_Data2DInstance* instances;
	uintsize instance_count;
}
typedef Render_Data2D;

struct Engine_Renderer3DPointLight
{
	vec3 position;
	
	float32 constant, linear, quadratic;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
}
typedef Engine_Renderer3DPointLight;

struct Engine_Renderer3DFlashlight
{
	vec3 position;
	vec3 direction;
	vec3 color;
	
	float32 constant, linear, quadratic;
	
	float32 inner_cutoff;
	float32 outer_cutoff;
}
typedef Engine_Renderer3DFlashlight;

struct Engine_Renderer3DModel
{
	mat4 transform;
	Asset_3DModel* asset;
	vec4 color;
}
typedef Engine_Renderer3DModel;

struct Engine_Renderer3DScene
{
	Asset_3DModel* light_model;
	vec3 dirlight;
	vec3 dirlight_color;
	
	uint32 shadow_fbo, shadow_depthmap;
	uint32 gbuffer, gbuffer_pos, gbuffer_norm, gbuffer_albedo, gbuffer_depth;
	
	int32 model_count;
	int32 point_light_count;
	int32 flashlights_count;
	
	Engine_Renderer3DModel* models;
	Engine_Renderer3DPointLight* point_lights;
	Engine_Renderer3DFlashlight* flashlights;
}
typedef Engine_Renderer3DScene;

#else //INTERNAL_TEST_OPENGL_NEWREN
// TODO(ljre): API
struct Render_Texture2DDesc
{
	// NOTE(ljre): If actual image data
	const void* pixels;
	int32 width, height;
	uint32 channels;
	
	// NOTE(ljre): If image data to be decoded
	const void* encoded_image;
	uintsize encoded_image_size;
	
	// NOTE(ljre): options
	bool generate_mipmaps;
	bool mag_linear;
	bool min_linear;
	bool srgb;
	bool float_components;
}
typedef Render_Texture2DDesc;

struct Render_ShaderDesc
{
	struct
	{
		const char* fragment_shader_source;
		const char* vertex_shader_source;
		
		uint8 attrib_vertex_position;
		uint8 attrib_vertex_normal;
		uint8 attrib_vertex_texcoord;
		
		uint8 attrib_color;
		uint8 attrib_texcoords;
		uint8 attrib_transform;
		
		char uniform_view_matrix[32];
		char uniform_texture_sampler[32];
	}
	opengl;
}
typedef Render_ShaderDesc;

struct Render_FontDesc
{
	const void* ttf_data;
	uintsize ttf_data_size;
	
	float32 char_height;
	int32 glyph_cache_size;
}
typedef Render_FontDesc;

struct Render_FramebufferDesc
{
	int32 width, height;
	
	Render_Texture2DDesc color_attachment;
	
	// NOTE(ljre): Only one of these 3 shall be used at once.
	Render_Texture2DDesc depth_attachment;
	Render_Texture2DDesc depth_stencil_attachment;
	Render_Texture2DDesc stencil_attachment;
}
typedef Render_FramebufferDesc;

struct Render_Texture2D
{
	int32 width, height;
	
	struct
	{
		uint32 id;
	}
	opengl;
}
typedef Render_Texture2D;

struct Render_Shader
{
	struct
	{
		uint32 id;
		int32 uniform_view_matrix_loc;
		int32 uniform_texture_sampler_loc;
		
		uint8 attrib_vertex_position;
		uint8 attrib_vertex_normal;
		uint8 attrib_vertex_texcoord;
		
		uint8 attrib_color;
		uint8 attrib_texcoords;
		uint8 attrib_transform;
	}
	opengl;
}
typedef Render_Shader;

struct Render_Font
{
	
	struct
	{
		uint8 dummy;
	}
	opengl;
}
typedef Render_Font;

struct Render_Framebuffer
{
	int32 width, height;
	
	Render_Texture2D color_attachment;
	
	// NOTE(ljre): Only one of these will be in use at once.
	Render_Texture2D depth_attachment;
	Render_Texture2D depth_stencil_attachment;
	Render_Texture2D stencil_attachment;
	
	struct
	{
		uint32 id;
	}
	opengl;
}
typedef Render_Framebuffer;

struct Render_DrawTextData
{
	const Render_Font* font;
	String text;
	vec4 color;
	
	vec2 pos;
	vec2 alignment; // xy = [0 ... 1]
	vec2 scale;
}
typedef Render_DrawTextData;

struct Render_Data2D
{
	const Render_Texture2D* texture;
	const Render_Shader* shader;
	const Render_Framebuffer* framebuffer;
	
	enum Render_BlendMode blendmode;
	Render_Camera2D camera;
	
	const Render_Data2DInstance* instances;
	uintsize instance_count;
}
typedef Render_Data2D;

#endif //INTERNAL_TEST_OPENGL_NEWREN

//~ Main API
struct Engine_RenderApi
{
#ifndef INTERNAL_TEST_OPENGL_NEWREN
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
	
#else //INTERNAL_TEST_OPENGL_NEWREN
	bool (*make_texture_2d)(const Render_Texture2DDesc* desc, Render_Texture2D* out_texture);
	bool (*make_font)(const Render_FontDesc* desc, Render_Font* out_font);
	bool (*make_shader)(const Render_ShaderDesc* desc, Render_Shader* out_shader);
	bool (*make_framebuffer)(const Render_FramebufferDesc* desc, Render_Framebuffer* out_fb);
	
	void (*free_texture_2d)(Render_Texture2D* texture);
	void (*free_font)(Render_Font* font);
	void (*free_shader)(Render_Shader* shader);
	void (*free_framebuffer)(Render_Framebuffer* fb);
	
	void (*clear_color)(const vec4 color);
	void (*clear_framebuffer)(Render_Framebuffer* fb, const vec4 color);
	
	void (*draw_2d)(const Render_Data2D* data);
	void (*draw_text)(const Render_DrawTextData* data);
	
	void (*calc_text_size)(const Render_Font* font, String text, vec2* out_size);
#endif //INTERNAL_TEST_OPENGL_NEWREN
}
typedef Engine_RenderApi;

//~ Wrappers
#ifndef INTERNAL_TEST_OPENGL_NEWREN

static inline void
Render_ClearColor(Engine_Data* engine, const vec4 color)
{ engine->render->clear_color(color); }

static inline void
Render_Draw2D(Engine_Data* engine, const Render_Data2D* data)
{ engine->render->draw_2d(data); }

#else //INTERNAL_TEST_OPENGL_NEWREN

static inline bool
Render_MakeTexture2D(Engine_Data* engine, const Render_Texture2DDesc* desc, Render_Texture2D* out_texture)
{ return engine->render->make_texture_2d(desc, out_texture); }

static inline bool
Render_MakeFont(Engine_Data* engine, const Render_FontDesc* desc, Render_Font* out_font)
{ return engine->render->make_font(desc, out_font); }

static inline bool
Render_MakeShader(Engine_Data* engine, const Render_ShaderDesc* desc, Render_Shader* out_shader)
{ return engine->render->make_shader(desc, out_shader); }

static inline bool
Render_MakeFramebuffer(Engine_Data* engine, const Render_FramebufferDesc* desc, Render_Framebuffer* out_fb)
{ return engine->render->make_framebuffer(desc, out_fb); }

static inline void
Render_FreeTexture2D(Engine_Data* engine, Render_Texture2D* texture)
{ engine->render->free_texture_2d(texture); }

static inline void
Render_FreeFont(Engine_Data* engine, Render_Font* font)
{ engine->render->free_font(font); }

static inline void
Render_FreeShader(Engine_Data* engine, Render_Shader* shader)
{ engine->render->free_shader(shader); }

static inline void
Render_FreeFramebuffer(Engine_Data* engine, Render_Framebuffer* fb)
{ engine->render->free_framebuffer(fb); }

static inline void
Render_ClearColor(Engine_Data* engine, const vec4 color)
{ engine->render->clear_color(color); }

static inline void
Render_ClearFramebuffer(Engine_Data* engine, Render_Framebuffer* fb, const vec4 color)
{ engine->render->clear_framebuffer(fb, color); }

static inline void
Render_Draw2D(Engine_Data* engine, const Render_Data2D* data)
{ engine->render->draw_2d(data); }

static inline void
Render_DrawText(Engine_Data* engine, const Render_DrawTextData* data)
{ engine->render->draw_text(data); }

static inline void
Render_CalcTextSize(Engine_Data* engine, const Render_Font* font, String text, vec2* out_size)
{ engine->render->calc_text_size(font, text, out_size); }

#endif //INTERNAL_TEST_OPENGL_NEWREN


#endif //INTERNAL_API_RENDER_H
