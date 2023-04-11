#ifndef API_RENDERBACKEND_H
#define API_RENDERBACKEND_H

enum
{
	RB_Limits_MaxTexturesPerDrawCall = 4,
	RB_Limits_MaxVertexInputs = 8,
	RB_Limits_ColorAttachPerRenderTarget = 4,
	RB_Limits_MaxVertexBuffersPerDrawCall = 4,
};

enum RB_ShaderType
{
	RB_ShaderType_Null = 0,
	
	RB_ShaderType_Glsl33,       // GLSL 3.3 (vertex & fragment) source code
	RB_ShaderType_Hlsl40,       // vs_4_0 & ps_4_0 object code
	RB_ShaderType_HlslLevel91,  // vs_4_0_level_9_1 & ps_4_0_level_9_1 object code
}
typedef RB_ShaderType;

enum RB_TexFormat
{
	RB_TexFormat_Null = 0,
	
	RB_TexFormat_D16,
	RB_TexFormat_D24S8,
	RB_TexFormat_A8,
	RB_TexFormat_R8,
	RB_TexFormat_RG8,
	RB_TexFormat_RGB8,
	RB_TexFormat_RGBA8,
	
	RB_TexFormat_Count,
}
typedef RB_TexFormat;

struct RB_Capabilities
{
	String backend_api;
	String driver_renderer;
	String driver_vendor;
	String driver_version;
	
	RB_ShaderType shader_type;
	int32 max_texture_size;
	int32 max_render_target_textures;
	int32 max_textures_per_drawcall;
	
	uint64 supported_texture_formats[(RB_TexFormat_Count + 63) / 64];
	
	bool has_instancing : 1;
	bool has_32bit_index : 1;
	bool has_separate_alpha_blend : 1;
	bool has_compute_shaders : 1;
	bool has_16bit_float : 1;
	bool has_wireframe_fillmode : 1;
}
typedef RB_Capabilities;

union RB_Handle
{ uint32 id; }
typedef RB_Handle;

//~ ResourceCommand
enum RB_LayoutDescKind
{
	RB_LayoutDescKind_Null = 0,
	
	RB_LayoutDescKind_Scalar,
	RB_LayoutDescKind_Vec2,
	RB_LayoutDescKind_Vec3,
	RB_LayoutDescKind_Vec4,
	RB_LayoutDescKind_Mat2,
	RB_LayoutDescKind_Mat3,
	RB_LayoutDescKind_Mat4,
	
	RB_LayoutDescKind_Vec2I16Norm,
	RB_LayoutDescKind_Vec2I16,
	RB_LayoutDescKind_Vec4I16Norm,
	RB_LayoutDescKind_Vec4I16,
	RB_LayoutDescKind_Vec4U8Norm,
	RB_LayoutDescKind_Vec4U8,
}
typedef RB_LayoutDescKind;

struct RB_LayoutDesc
{
	RB_LayoutDescKind kind;
	uint32 offset;
	
	uint8 divisor;
	uint8 vbuffer_index;
}
typedef RB_LayoutDesc;

enum RB_BlendFunc
{
	RB_BlendFunc_Null = 0,
	
	RB_BlendFunc_Zero,
	RB_BlendFunc_One,
	RB_BlendFunc_SrcColor,
	RB_BlendFunc_InvSrcColor,
	RB_BlendFunc_DstColor,
	RB_BlendFunc_InvDstColor,
	RB_BlendFunc_SrcAlpha,
	RB_BlendFunc_InvSrcAlpha,
	RB_BlendFunc_DstAlpha,
	RB_BlendFunc_InvDstAlpha,
}
typedef RB_BlendFunc;

enum RB_BlendOp
{
	RB_BlendOp_Add = 0,
	RB_BlendOp_Subtract,
}
typedef RB_BlendOp;

enum RB_FillMode
{
	RB_FillMode_Solid = 0,
	RB_FillMode_Wireframe,
}
typedef RB_FillMode;

enum RB_CullMode
{
	RB_CullMode_None = 0,
	RB_CullMode_Front,
	RB_CullMode_Back,
}
typedef RB_CullMode;

enum RB_IndexType
{
	RB_IndexType_Uint32 = 0,
	RB_IndexType_Uint16,
}
typedef RB_IndexType;

enum RB_ResourceCommandKind
{
	RB_ResourceCommandKind_Null = 0,
	
	RB_ResourceCommandKind_MakeTexture2D,
	RB_ResourceCommandKind_MakeVertexBuffer,
	RB_ResourceCommandKind_MakeIndexBuffer,
	RB_ResourceCommandKind_MakeUniformBuffer,
	RB_ResourceCommandKind_MakeShader,
	RB_ResourceCommandKind_MakeRenderTarget,
	RB_ResourceCommandKind_MakePipeline,
	//
	RB_ResourceCommandKind_UpdateVertexBuffer,
	RB_ResourceCommandKind_UpdateIndexBuffer,
	RB_ResourceCommandKind_UpdateUniformBuffer,
	RB_ResourceCommandKind_UpdateTexture2D,
	//
	RB_ResourceCommandKind_FreeTexture2D,
	RB_ResourceCommandKind_FreeVertexBuffer,
	RB_ResourceCommandKind_FreeIndexBuffer,
	RB_ResourceCommandKind_FreeUniformBuffer,
	RB_ResourceCommandKind_FreeShader,
	RB_ResourceCommandKind_FreeRenderTarget,
	RB_ResourceCommandKind_FreePipeline,
}
typedef RB_ResourceCommandKind;

struct RB_ResourceCommand typedef RB_ResourceCommand;
struct RB_ResourceCommand
{
	RB_ResourceCommand* next;
	RB_ResourceCommandKind kind;
	bool flag_dynamic : 1;
	bool flag_subregion : 1;
	
	RB_Handle* handle;
	
	union
	{
		struct
		{
			const void* pixels;
			int32 width, height;
			RB_TexFormat format;
			
			bool flag_linear_filtering : 1;
			bool flag_render_target : 1;
			bool flag_not_used_in_shader : 1;
			
			// used if 'flag_subregion' is set and not on init.
			int32 xoffset;
			int32 yoffset;
		}
		texture_2d;
		
		struct
		{
			Buffer d3d_vs40_blob, d3d_ps40_blob;
			Buffer d3d_vs40level91_blob, d3d_ps40level91_blob;
			String gl_vs_src, gl_fs_src;
		}
		shader;
		
		struct
		{
			bool flag_blend : 1;
			bool flag_cw_backface : 1;
			bool flag_depth_test : 1;
			bool flag_scissor : 1;
			
			RB_BlendFunc blend_source;
			RB_BlendFunc blend_dest;
			RB_BlendOp blend_op;
			RB_BlendFunc blend_source_alpha;
			RB_BlendFunc blend_dest_alpha;
			RB_BlendOp blend_op_alpha;
			
			RB_FillMode fill_mode;
			RB_CullMode cull_mode;
			
			RB_Handle* shader;
			RB_LayoutDesc input_layout[RB_Limits_MaxVertexInputs];
		}
		pipeline;
		
		struct
		{
			const void* memory;
			uintsize size;
			
			// used if 'flag_subregion' is set and not on init.
			uintsize offset;
			
			// used if on 'RB_ResourceCommandKind_MakeIndexBuffer'.
			RB_IndexType index_type;
		}
		buffer;
		
		struct
		{
			RB_Handle* color_textures[RB_Limits_ColorAttachPerRenderTarget];
			RB_Handle* depth_stencil_texture;
		}
		render_target;
	};
};

//~ DrawCommand
enum RB_DrawCommandKind
{
	RB_DrawCommandKind_Null = 0,
	
	RB_DrawCommandKind_Clear,
	RB_DrawCommandKind_ApplyPipeline,
	RB_DrawCommandKind_ApplyRenderTarget,
	RB_DrawCommandKind_DrawIndexed,
	RB_DrawCommandKind_DrawInstanced,
}
typedef RB_DrawCommandKind;

struct RB_DrawCommand typedef RB_DrawCommand;
struct RB_DrawCommand
{
	RB_DrawCommand* next;
	RB_ResourceCommand* resources_cmd;
	RB_DrawCommandKind kind;
	
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
			RB_Handle* handle; // if NULL, then choose default
		}
		apply;
		
		struct
		{
			RB_Handle* ibuffer;
			RB_Handle* ubuffer;
			RB_Handle* textures[RB_Limits_MaxTexturesPerDrawCall];
			RB_Handle* vbuffers[RB_Limits_MaxVertexBuffersPerDrawCall];
			uint32 vbuffer_strides[RB_Limits_MaxVertexBuffersPerDrawCall];
			uint32 vbuffer_offsets[RB_Limits_MaxVertexBuffersPerDrawCall];
			
			uint32 base_index;
			uint32 index_count;
			
			// used if on 'RB_DrawCommandKind_DrawInstanced'.
			uint32 instance_count;
		}
		draw_instanced;
	};
};

API void RB_Init(Arena* scratch_arena, const OS_WindowGraphicsContext* graphics_context);
API void RB_Deinit(Arena* scratch_arena);
API bool RB_Present(Arena* scratch_arena, int32 vsync_count);

API void RB_QueryCapabilities(RB_Capabilities* out_capabilities);
API void RB_ExecuteResourceCommands(Arena* scratch_arena, RB_ResourceCommand* commands);
API void RB_ExecuteDrawCommands(Arena* scratch_arena, RB_DrawCommand* commands, int32 default_width, int32 default_height);

#endif //API_RENDERBACKEND_H
