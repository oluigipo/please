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
}
typedef Render_BlendMode;

struct Render_Camera2D
{
	vec2 pos;
	vec2 size;
	float32 zoom;
	float32 angle;
}
typedef Render_Camera2D;

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
		const void* ps_blob_data;
		uintsize ps_blob_size;
		const void* vs_blob_data;
		uintsize vs_blob_size;
	}
	d3d11;
	
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
	Arena* arena;
	
	const void* ttf_data;
	uintsize ttf_data_size;
	
	float32 char_height;
	int32 glyph_cache_size; // NOTE(ljre): This must be the log2(sqrt()) of the size!
	
	bool mag_linear;
	bool min_linear;
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
	
	union
	{
		struct { uint32 id;    } opengl;
		struct { void* texture_handle; void* view_handle; } d3d11;
	};
}
typedef Render_Texture2D;

struct Render_Shader
{
	union
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
		
		struct
		{
			void* vs_handle;
			void* ps_handle;
		}
		d3d11;
	};
}
typedef Render_Shader;

struct Render_GlyphInfo
{
	uint32 codepoint;
	int32 index;
	
	int16 xoff, yoff;
	uint16 width, height;
	uint16 advance_width, left_side_bearing;
}
typedef Render_GlyphInfo;

struct Render_Font
{
	const void* ttf_data;
	uintsize ttf_data_size;
	
	stbtt_fontinfo info;
	int32 ascent, descent, line_gap, space_char_advance;
	float32 char_scale;
	
	int32 max_glyph_width, max_glyph_height;
	uint8* cache_bitmap;
	int32 cache_bitmap_size; // NOTE(ljre): Square bitmap, size = width/max_glyph_width and height/max_glyph_height.
	
	Render_GlyphInfo* cache_hashtable;
	uint32 cache_hashtable_log2_cap;
	uint32 cache_hashtable_size;
	
	Render_Texture2D cache_tex;
	
	struct
	{
		uint32 cache_tex_id;
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

struct Render_Data2D
{
	const Render_Texture2D* texture;
	const Render_Shader* shader;
	const Render_Framebuffer* framebuffer;
	
	Render_BlendMode blendmode;
	Render_Camera2D camera;
	
	const Render_Data2DInstance* instances;
	uintsize instance_count;
}
typedef Render_Data2D;

//~ Main API
struct E_RenderApi
{
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
	void (*batch_text)(Render_Font* font, String text, const vec4 color, const vec2 pos, const vec2 alignment, const vec2 scale, Arena* arena, Render_Data2D* out_data);
	
	void (*calc_text_size)(const Render_Font* font, String text, vec2* out_size);
}
typedef E_RenderApi;

//~ Wrappers
static inline bool
Render_MakeTexture2D(E_GlobalData* engine, const Render_Texture2DDesc* desc, Render_Texture2D* out_texture)
{ return engine->render->make_texture_2d(desc, out_texture); }

static inline bool
Render_MakeFont(E_GlobalData* engine, const Render_FontDesc* desc, Render_Font* out_font)
{ return engine->render->make_font(desc, out_font); }

static inline bool
Render_MakeShader(E_GlobalData* engine, const Render_ShaderDesc* desc, Render_Shader* out_shader)
{ return engine->render->make_shader(desc, out_shader); }

static inline bool
Render_MakeFramebuffer(E_GlobalData* engine, const Render_FramebufferDesc* desc, Render_Framebuffer* out_fb)
{ return engine->render->make_framebuffer(desc, out_fb); }

static inline void
Render_FreeTexture2D(E_GlobalData* engine, Render_Texture2D* texture)
{ engine->render->free_texture_2d(texture); }

static inline void
Render_FreeFont(E_GlobalData* engine, Render_Font* font)
{ engine->render->free_font(font); }

static inline void
Render_FreeShader(E_GlobalData* engine, Render_Shader* shader)
{ engine->render->free_shader(shader); }

static inline void
Render_FreeFramebuffer(E_GlobalData* engine, Render_Framebuffer* fb)
{ engine->render->free_framebuffer(fb); }

static inline void
Render_ClearColor(E_GlobalData* engine, const vec4 color)
{ engine->render->clear_color(color); }

static inline void
Render_ClearFramebuffer(E_GlobalData* engine, Render_Framebuffer* fb, const vec4 color)
{ engine->render->clear_framebuffer(fb, color); }

static inline void
Render_Draw2D(E_GlobalData* engine, const Render_Data2D* data)
{ engine->render->draw_2d(data); }

static inline void
Render_BatchText(E_GlobalData* engine, Render_Font* font, String text, const vec4 color, const vec2 pos, const vec2 alignment, const vec2 scale, Arena* arena, Render_Data2D* out_data)
{ engine->render->batch_text(font, text, color, pos, alignment, scale, arena, out_data); }

static inline void
Render_CalcTextSize(E_GlobalData* engine, const Render_Font* font, String text, vec2* out_size)
{ engine->render->calc_text_size(font, text, out_size); }

#endif //INTERNAL_API_RENDER_H
