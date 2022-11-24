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
#if 0
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
			uint32 uniform_view_matrix_loc;
			uint32 uniform_texture_sampler_loc;
			
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
			
		}
		opengl;
	}
	typedef Render_Font;
	
	bool (*make_texture_2d_from_pixels)(const Render_Texture2DDesc* desc, Render_Texture2D* out_texture);
	bool (*make_font)(const Render_FontDesc* desc, Render_Font* out_font);
	bool (*make_shader)(const Render_ShaderDesc* desc, Render_Shader* out_shader);
	
	void (*clear_color)(const vec4 color);
	void (*draw_2d)(const Render_Data2D* batch);
	
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
	
	void (*draw_text)(const Render_DrawTextData* data);
	void (*calc_text_size)(const Render_Font* font, String text, vec2* out_size);
	
	void (*free_texture_2d)(Render_Texture2D* texture);
	void (*free_font)(Render_Font* font);
	void (*free_shader)(Render_Shader* shader);
#else
	void (*clear_color)(const vec4 color);
	void (*draw_2d)(const Render_Data2D* data);
#endif
	
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
static inline void
Render_ClearColor(Engine_Data* engine, const vec4 color)
{ engine->render->clear_color(color); }

static inline void
Render_Draw2D(Engine_Data* engine, const Render_Data2D* data)
{ engine->render->draw_2d(data); }

#endif //INTERNAL_API_RENDER_H
