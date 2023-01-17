#ifndef RENDER_BACKEND_API_H
#define RENDER_BACKEND_API_H

union RenderBackend_Handle { void* ref; uint32 id; } typedef RenderBackend_Handle;

//~ ResourceCommand
enum RenderBackend_ResourceCommandKind
{
	RenderBackend_ResourceCommandKind_Null = 0,
	
	RenderBackend_ResourceCommandKind_MakeTexture2D,
	RenderBackend_ResourceCommandKind_MakeVertexBuffer,
	RenderBackend_ResourceCommandKind_MakeIndexBuffer,
	RenderBackend_ResourceCommandKind_MakeShader,
	RenderBackend_ResourceCommandKind_MakeRenderTarget,
	//
	RenderBackend_ResourceCommandKind_UpdateVertexBuffer,
	RenderBackend_ResourceCommandKind_UpdateIndexBuffer,
	RenderBackend_ResourceCommandKind_UpdateTexture2D,
	//
	RenderBackend_ResourceCommandKind_FreeTexture2D,
	RenderBackend_ResourceCommandKind_FreeVertexBuffer,
	RenderBackend_ResourceCommandKind_FreeIndexBuffer,
	RenderBackend_ResourceCommandKind_FreeShader,
	RenderBackend_ResourceCommandKind_FreeRenderTarget,
}
typedef RenderBackend_ResourceCommandKind;

struct RenderBackend_ResourceCommand typedef RenderBackend_ResourceCommand;
struct RenderBackend_ResourceCommand
{
	RenderBackend_ResourceCommand* next;
	RenderBackend_ResourceCommandKind kind;
	bool flag_dynamic : 1;
	bool flag_subregion : 1;
	
	RenderBackend_Handle* handle;
	
	union
	{
		struct
		{
			const uint8* pixels;
			int32 width, height;
			uint32 channels;
			
			// used if 'flag_subregion' is set and not on init.
			int32 xoffset;
			int32 yoffset;
		}
		texture_2d;
		
		struct
		{
			Buffer d3d_vs_blob, d3d_ps_blob;
			String gl_vs_src, gl_fs_src;
		}
		shader;
		
		struct
		{
			Buffer memory;
			
			// used if 'flag_subregion' is set and not on init.
			uintsize offset;
		}
		buffer;
		
		struct
		{
			RenderBackend_Handle* color_textures[4];
			RenderBackend_Handle* depth_stencil_texture;
		}
		render_target;
	};
};

//~ DrawCommand
enum RenderBackend_LayoutDescKind
{
	RenderBackend_LayoutDescKind_Null = 0,
	
	RenderBackend_LayoutDescKind_Vec2,
	RenderBackend_LayoutDescKind_Vec3,
	RenderBackend_LayoutDescKind_Vec4,
	RenderBackend_LayoutDescKind_Mat4,
}
typedef RenderBackend_LayoutDescKind;

struct RenderBackend_LayoutDesc
{
	RenderBackend_LayoutDescKind kind;
	uint32 stride;
	uint32 offset;
	
	uint8 location;
	uint8 divisor;
	uint8 vbuffer_index;
}
typedef RenderBackend_LayoutDesc;

struct RenderBackend_SamplerDesc
{
	RenderBackend_Handle* handle;
	uint32 d3d_id;
	String gl_name;
}
typedef RenderBackend_SamplerDesc;

enum RenderBackend_DrawCommandKind
{
	RenderBackend_DrawCommandKind_Null = 0,
	
	RenderBackend_DrawCommandKind_Clear,
	RenderBackend_DrawCommandKind_SetRenderTarget,
	RenderBackend_DrawCommandKind_ResetRenderTarget,
	RenderBackend_DrawCommandKind_DrawCall,
}
typedef RenderBackend_DrawCommandKind;

struct RenderBackend_DrawCommand typedef RenderBackend_DrawCommand;
struct RenderBackend_DrawCommand
{
	RenderBackend_DrawCommand* next;
	RenderBackend_ResourceCommand* resources_cmd;
	RenderBackend_DrawCommandKind kind;
	
	union
	{
		struct
		{
			float32 color[4];
			
			bool flag_color : 1;
			bool flag_depth : 1;
			bool flag_stencil : 1;
		}
		clear;
		
		struct
		{
			RenderBackend_Handle* handle;
		}
		set_target;
		
		struct
		{
			RenderBackend_Handle* shader;
			RenderBackend_Handle* vbuffers[4];
			RenderBackend_Handle* ibuffer;
			
			uint32 index_count;
			uint32 instance_count;
			
			Buffer uniform_buffer;
			
			RenderBackend_SamplerDesc samplers[8];
			RenderBackend_LayoutDesc vertex_layout[16];
		}
		drawcall;
	};
};

API void RenderBackend_Init(Arena* scratch_arena, const OS_WindowGraphicsContext* graphics_context);
API void RenderBackend_Deinit(Arena* scratch_arena);
API bool RenderBackend_Present(Arena* scratch_arena, int32 vsync_count);

API void RenderBackend_ExecuteResourceCommands(Arena* scratch_arena, RenderBackend_ResourceCommand* commands);
API void RenderBackend_ExecuteDrawCommands(Arena* scratch_arena, RenderBackend_DrawCommand* commands);

#endif //RENDER_BACKEND_API_H
