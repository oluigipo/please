#ifndef API_RENDERBACKEND_H
#define API_RENDERBACKEND_H

// NOTE(ljre): Sketching new renderbackend API. The idea of command lists, while making the API impl a bit
//             simpler by deferring all API calls to 2 function calls, can be a little hard to understand.
//
// Here are the key differences between this and the old one:
//    - No command lists, just plain function calls;
//    - One handle type per resource;
//    - No global state is needed;
//    - Still using RB_*Desc keeps all simplicity of old API while still allowing for deferred render with
//      little to no changes if needed;
//    - Because no global state is needed, making a "reset" function is trivial. This allows us to
//      change some fundamental settings in the renderer without reopening the application;
//
// Current version of the old one is in commit 5d1e281f563c4d5f62f44b31bd1ab08b4bba9a9b

// NOTE(ljre): This API is obviously inspired by sokol_gfx. I'm just not using it because I wanna learn
//             how to use the underlying graphics APIs.

enum
{
	RB_Limits_DrawMaxTextures = 8,
	RB_Limits_DrawMaxVertexBuffers = 8,
	RB_Limits_PipelineMaxVertexInputs = 16,
	RB_Limits_RenderTargetMaxColorAttachments = 4,
};

struct RB_Ctx typedef RB_Ctx;
struct RB_Tex2d { uint32 id; } typedef RB_Tex2d;
struct RB_VBuffer { uint32 id; } typedef RB_VBuffer;
struct RB_IBuffer { uint32 id; } typedef RB_IBuffer;
struct RB_UBuffer { uint32 id; } typedef RB_UBuffer;
struct RB_RenderTarget { uint32 id; } typedef RB_RenderTarget;
struct RB_Shader { uint32 id; } typedef RB_Shader;
struct RB_Pipeline { uint32 id; } typedef RB_Pipeline;

//~
enum RB_ShaderType
{
	RB_ShaderType_Null = 0,
	
	RB_ShaderType_Glsl,           // GLSL (vertex & fragment) source code for both GL3.3 and GLES3.0
	RB_ShaderType_Hlsl40Level91,  // vs_4_0_level_9_1 & ps_4_0_level_9_1 object code
	RB_ShaderType_Hlsl40Level93,  // vs_4_0_level_9_3 & ps_4_0_level_9_3 object code
	RB_ShaderType_Hlsl40,         // vs_4_0 & ps_4_0 object code
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

API RB_Ctx* RB_MakeContext(Arena* arena, const OS_WindowGraphicsContext* graphics_context);
API RB_Ctx* RB_MakeDeferredContext(RB_Ctx* parent, Arena* );
API void RB_FreeContext(RB_Ctx* ctx);
API void RB_Present(RB_Ctx* ctx);
#define RB_IsNull(handle) (!(handle).id)
#define RB_IsValid(ctx, handle) RB_IsValidImpl(ctx, (handle).id)
#define RB_IsSame(ctx, handle1, handle2) RB_IsSameImpl(ctx, (handle1).id, (handle2).id)
API bool RB_IsValidImpl(RB_Ctx* ctx, uint32 handle);
API bool RB_IsSameImpl(RB_Ctx* ctx, uint32 id1, uint32 id2);
API RB_Capabilities RB_QueryCapabilities(RB_Ctx* ctx);

//~
enum RB_VertexFormat
{
	RB_VertexFormat_Null = 0,
	
	RB_VertexFormat_Scalar,
	RB_VertexFormat_Vec2,
	RB_VertexFormat_Vec3,
	RB_VertexFormat_Vec4,
	RB_VertexFormat_Mat2,
	RB_VertexFormat_Mat3,
	RB_VertexFormat_Mat4,
	
	RB_VertexFormat_Vec2I16Norm,
	RB_VertexFormat_Vec2I16,
	RB_VertexFormat_Vec2F16,
	RB_VertexFormat_Vec4I16Norm,
	RB_VertexFormat_Vec4I16,
	RB_VertexFormat_Vec4F16,
	RB_VertexFormat_Vec4U8Norm,
	RB_VertexFormat_Vec4U8,
}
typedef RB_VertexFormat;

struct RB_LayoutDesc
{
	uint32 offset;
	uint8 buffer_slot;
	RB_VertexFormat format : 8;
	uint8 divisor;
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
	RB_IndexType_Null = 0,
	
	RB_IndexType_Uint16,
	RB_IndexType_Uint32,
}
typedef RB_IndexType;

struct RB_Tex2dDesc
{
	const void* pixels;
	int32 width, height;
	RB_TexFormat format;
	
	bool flag_dynamic : 1;
	// NOTE(ljre): Maybe move to RB_DrawDesc or something?
	bool flag_linear_filtering : 1;
	bool flag_render_target : 1;
	// NOTE(ljre): Should be marked as 'true' for depth formats only used in render targets
	bool flag_not_used_in_shader : 1;
}
typedef RB_Tex2dDesc;

struct RB_ShaderDesc
{
	struct
	{
		String vs;
		String fs;
	}
	glsl;
	struct
	{
		Buffer vs;
		Buffer ps;
	}
	hlsl40, hlsl40_93, hlsl40_91;
}
typedef RB_ShaderDesc;

struct RB_PipelineDesc
{
	bool flag_blend : 1;
	bool flag_cw_backface : 1;
	bool flag_depth_test : 1;
	//bool flag_scissor : 1;
	
	RB_BlendFunc blend_source;
	RB_BlendFunc blend_dest;
	RB_BlendOp blend_op;
	RB_BlendFunc blend_source_alpha;
	RB_BlendFunc blend_dest_alpha;
	RB_BlendOp blend_op_alpha;
	
	RB_FillMode fill_mode;
	RB_CullMode cull_mode;
	
	RB_Shader shader;
	RB_LayoutDesc input_layout[RB_Limits_PipelineMaxVertexInputs];
}
typedef RB_PipelineDesc;

struct RB_VBufferDesc
{
	bool flag_dynamic : 1;
	
	uintsize size;
	const void* initial_data;
}
typedef RB_VBufferDesc;

struct RB_IBufferDesc
{
	bool flag_dynamic : 1;
	
	uintsize size;
	const void* initial_data;
	RB_IndexType index_type;
}
typedef RB_IBufferDesc;

struct RB_UBufferDesc
{
	uintsize size;
	const void* initial_data;
}
typedef RB_UBufferDesc;

struct RB_RenderTargetDesc
{
	RB_Tex2d color[RB_Limits_RenderTargetMaxColorAttachments];
	RB_Tex2d depth_stencil;
}
typedef RB_RenderTargetDesc;

API RB_Tex2d        RB_MakeTexture2D(RB_Ctx* ctx, const RB_Tex2dDesc* desc);
API RB_VBuffer      RB_MakeVertexBuffer(RB_Ctx* ctx, const RB_VBufferDesc* desc);
API RB_IBuffer      RB_MakeIndexBuffer(RB_Ctx* ctx, const RB_IBufferDesc* desc);
API RB_UBuffer      RB_MakeUniformBuffer(RB_Ctx* ctx, const RB_UBufferDesc* desc);
API RB_Shader       RB_MakeShader(RB_Ctx* ctx, const RB_ShaderDesc* desc);
API RB_RenderTarget RB_MakeRenderTarget(RB_Ctx* ctx, const RB_RenderTargetDesc* desc);
API RB_Pipeline     RB_MakePipeline(RB_Ctx* ctx, const RB_PipelineDesc* desc);

API void RB_FreeTexture2D(RB_Ctx* ctx, RB_Tex2d res);
API void RB_FreeVertexBuffer(RB_Ctx* ctx, RB_VBuffer res);
API void RB_FreeIndexBuffer(RB_Ctx* ctx, RB_IBuffer res);
API void RB_FreeUniformBuffer(RB_Ctx* ctx, RB_UBuffer res);
API void RB_FreeShader(RB_Ctx* ctx, RB_Shader res);
API void RB_FreeRenderTarget(RB_Ctx* ctx, RB_RenderTarget res);
API void RB_FreePipeline(RB_Ctx* ctx, RB_Pipeline res);

API void RB_UpdateVertexBuffer(RB_Ctx* ctx, RB_VBuffer res, Buffer new_data);
API void RB_UpdateIndexBuffer(RB_Ctx* ctx, RB_IBuffer res, Buffer new_data);
API void RB_UpdateUniformBuffer(RB_Ctx* ctx, RB_UBuffer res, Buffer new_data);
API void RB_UpdateTexture2D(RB_Ctx* ctx, RB_Tex2d res, Buffer new_data);

//~
struct RB_BeginDesc
{
	int32 viewport_width;
	int32 viewport_height;
}
typedef RB_BeginDesc;

struct RB_ClearDesc
{
	float32 color[4];
	
	bool flag_color : 1;
	bool flag_depth : 1;
	bool flag_stencil : 1;
}
typedef RB_ClearDesc;

struct RB_DrawDesc
{
	RB_IBuffer ibuffer;
	RB_UBuffer ubuffer;
	RB_Tex2d textures[RB_Limits_DrawMaxTextures];
	
	RB_VBuffer vbuffers[RB_Limits_DrawMaxVertexBuffers];
	uint32 strides[RB_Limits_DrawMaxVertexBuffers];
	uint32 offsets[RB_Limits_DrawMaxVertexBuffers];
	
	uint32 base_index;
	uint32 index_count;
	uint32 instance_count; // if 0, not instanced
}
typedef RB_DrawDesc;

API void RB_BeginCmd(RB_Ctx* ctx, const RB_BeginDesc* desc);
API void RB_CmdApplyPipeline(RB_Ctx* ctx, RB_Pipeline pipeline);
API void RB_CmdApplyRenderTarget(RB_Ctx* ctx, RB_RenderTarget render_target);
API void RB_CmdClear(RB_Ctx* ctx, const RB_ClearDesc* desc);
API void RB_CmdDraw(RB_Ctx* ctx, const RB_DrawDesc* desc);
API void RB_EndCmd(RB_Ctx* ctx);

#endif //API_RENDERBACKEND_H
