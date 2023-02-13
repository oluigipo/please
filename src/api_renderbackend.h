#ifndef API_RENDERBACKEND_H
#define API_RENDERBACKEND_H

enum
{
	RB_Limits_SamplersPerDrawCall = 4,
	RB_Limits_InputsPerShader = 8,
	RB_Limits_ColorAttachPerRenderTarget = 4,
	RB_Limits_MaxVertexBuffersPerDrawCall = 4,
};

struct RB_Capabilities
{
	String backend_api;
	String driver;
	
	int32 max_texture_size;
}
typedef RB_Capabilities;

union RB_Handle
{ uint32 id; }
typedef RB_Handle;

//~ ResourceCommand
enum RB_LayoutDescKind
{
	RB_LayoutDescKind_Null = 0,
	
	RB_LayoutDescKind_Float,
	RB_LayoutDescKind_Vec2,
	RB_LayoutDescKind_Vec3,
	RB_LayoutDescKind_Vec4,
	RB_LayoutDescKind_Mat2,
	RB_LayoutDescKind_Mat3,
	RB_LayoutDescKind_Mat4,
}
typedef RB_LayoutDescKind;

struct RB_LayoutDesc
{
	RB_LayoutDescKind kind;
	uint32 offset;
	
	uint8 divisor;
	uint8 vbuffer_index;
	uint8 gl_location;
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
	RB_BlendOp_Null = 0,
	
	RB_BlendOp_Add,
	RB_BlendOp_Subtract,
}
typedef RB_BlendOp;

enum RB_FillMode
{
	RB_FillMode_Null = 0,
	
	RB_FillMode_Solid,
	RB_FillMode_Wireframe,
}
typedef RB_FillMode;

enum RB_CullMode
{
	RB_CullMode_Null = 0,
	
	RB_CullMode_None,
	RB_CullMode_Front,
	RB_CullMode_Back,
}
typedef RB_CullMode;

enum RB_ResourceCommandKind
{
	RB_ResourceCommandKind_Null = 0,
	
	RB_ResourceCommandKind_MakeTexture2D,
	RB_ResourceCommandKind_MakeVertexBuffer,
	RB_ResourceCommandKind_MakeIndexBuffer,
	RB_ResourceCommandKind_MakeUniformBuffer,
	RB_ResourceCommandKind_MakeShader,
	RB_ResourceCommandKind_MakeRenderTarget,
	RB_ResourceCommandKind_MakeBlendState,
	RB_ResourceCommandKind_MakeRasterizerState,
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
	RB_ResourceCommandKind_FreeBlendState,
	RB_ResourceCommandKind_FreeRasterizerState,
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
			uint32 channels;
			
			bool flag_linear_filtering : 1;
			
			// used if 'flag_subregion' is set and not on init.
			int32 xoffset;
			int32 yoffset;
		}
		texture_2d;
		
		struct
		{
			Buffer d3d_vs_blob, d3d_ps_blob;
			String gl_vs_src, gl_fs_src;
			
			RB_LayoutDesc input_layout[RB_Limits_InputsPerShader];
		}
		shader;
		
		struct
		{
			const void* memory;
			uintsize size;
			
			// used if 'flag_subregion' is set and not on init.
			uintsize offset;
		}
		buffer;
		
		struct
		{
			RB_Handle* color_textures[RB_Limits_ColorAttachPerRenderTarget];
			RB_Handle* depth_stencil_texture;
		}
		render_target;
		
		struct
		{
			bool enable;
			RB_BlendFunc source;
			RB_BlendFunc dest;
			RB_BlendOp op;
			RB_BlendFunc source_alpha;
			RB_BlendFunc dest_alpha;
			RB_BlendOp op_alpha;
		}
		blend;
		
		struct
		{
			RB_FillMode fill_mode;
			RB_CullMode cull_mode;
			
			bool flag_cw_backface : 1;
			bool flag_depth_test : 1;
			bool flag_scissor : 1;
		}
		rasterizer;
	};
};

//~ DrawCommand
struct RB_SamplerDesc
{
	RB_Handle* handle;
}
typedef RB_SamplerDesc;

enum RB_IndexType
{
	RB_IndexType_Uint32 = 0,
	RB_IndexType_Uint16, // apparently gltf needs this
	//RB_IndexType_Uint8, // but now this?????
}
typedef RB_IndexType;

enum RB_DrawCommandKind
{
	RB_DrawCommandKind_Null = 0,
	
	RB_DrawCommandKind_Clear,
	RB_DrawCommandKind_ApplyBlendState,
	RB_DrawCommandKind_ApplyRasterizerState,
	RB_DrawCommandKind_ApplyRenderTarget,
	RB_DrawCommandKind_DrawCall,
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
			RB_Handle* shader;
			RB_Handle* ibuffer;
			RB_Handle* ubuffer;
			RB_Handle* vbuffers[RB_Limits_MaxVertexBuffersPerDrawCall];
			uint32 vbuffer_strides[RB_Limits_MaxVertexBuffersPerDrawCall];
			uint32 vbuffer_offsets[RB_Limits_MaxVertexBuffersPerDrawCall];
			
			RB_IndexType index_type;
			uint32 index_count;
			uint32 instance_count;
			
			RB_SamplerDesc samplers[RB_Limits_SamplersPerDrawCall];
		}
		drawcall;
	};
};

API void RB_Init(Arena* scratch_arena, const OS_WindowGraphicsContext* graphics_context);
API void RB_Deinit(Arena* scratch_arena);
API bool RB_Present(Arena* scratch_arena, int32 vsync_count);

API void RB_QueryCapabilities(RB_Capabilities* out_capabilities);
API void RB_ExecuteResourceCommands(Arena* scratch_arena, RB_ResourceCommand* commands);
API void RB_ExecuteDrawCommands(Arena* scratch_arena, RB_DrawCommand* commands, int32 default_width, int32 default_height);

#endif //API_RENDERBACKEND_H
