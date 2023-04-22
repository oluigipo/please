#include "config.h"
#include "api_os.h"
#include "api_renderbackend.h"

struct RB_GenericHandle_ { uint32 id; } typedef RB_GenericHandle_;

enum RB_ResourceKind_
{
	RB_ResourceKind_Null_ = 0,
	
	RB_ResourceKind_MakeTexture2D_,
	RB_ResourceKind_MakeVertexBuffer_,
	RB_ResourceKind_MakeIndexBuffer_,
	RB_ResourceKind_MakeUniformBuffer_,
	RB_ResourceKind_MakeShader_,
	RB_ResourceKind_MakeRenderTarget_,
	RB_ResourceKind_MakePipeline_,
	
	RB_ResourceKind_UpdateTexture2D_,
	RB_ResourceKind_UpdateVertexBuffer_,
	RB_ResourceKind_UpdateIndexBuffer_,
	RB_ResourceKind_UpdateUniformBuffer_,
	
	RB_ResourceKind_FreeTexture2D_,
	RB_ResourceKind_FreeVertexBuffer_,
	RB_ResourceKind_FreeIndexBuffer_,
	RB_ResourceKind_FreeUniformBuffer_,
	RB_ResourceKind_FreeShader_,
	RB_ResourceKind_FreeRenderTarget_,
	RB_ResourceKind_FreePipeline_,
}
typedef RB_ResourceKind_;

enum RB_CommandKind_
{
	RB_CommandKind_Null_ = 0,
	
	RB_CommandKind_Begin_,
	RB_CommandKind_ApplyPipeline_,
	RB_CommandKind_ApplyRenderTarget_,
	RB_CommandKind_Clear_,
	RB_CommandKind_Draw_,
	RB_CommandKind_End_,
}
typedef RB_CommandKind_;

struct RB_ResourceCall_
{
	RB_ResourceKind_ kind;
	uint32* handle;
	
	union
	{
		RB_Tex2dDesc tex2d;
		RB_VBufferDesc vbuffer;
		RB_IBufferDesc ibuffer;
		RB_UBufferDesc ubuffer;
		RB_ShaderDesc shader;
		RB_RenderTargetDesc render_target;
		RB_PipelineDesc pipeline;
		
		struct { Buffer new_data; } update;
	};
}
typedef RB_ResourceCall_;

struct RB_CommandCall_
{
	RB_CommandKind_ kind;
	
	union
	{
		RB_BeginDesc begin;
		struct { uint8 dummy; } end;
		struct { RB_Pipeline handle; } apply_pipeline;
		struct { RB_RenderTarget handle; } apply_render_target;
		RB_ClearDesc clear;
		RB_DrawDesc draw;
	};
}
typedef RB_CommandCall_;

struct RB_Ctx
{
	Arena* arena;
	const OS_WindowGraphicsContext* graphics_context;
	RB_Capabilities caps;
	
	void* rt;
	void (*rt_free_ctx)(RB_Ctx* ctx);
	bool (*rt_is_valid_handle)(RB_Ctx* ctx, uint32 handle);
	void (*rt_resource)(RB_Ctx* ctx, const RB_ResourceCall_* resource);
	void (*rt_cmd)(RB_Ctx* ctx, const RB_CommandCall_* command);
};

#define RB_PoolAlloc_(pool, out_index) RB_PoolAllocImpl_(pool, ArrayLength((pool)->data), sizeof((pool)->data[0]), out_index)
#define RB_PoolFetch_(pool, index) RB_PoolFetchImpl_(pool, ArrayLength((pool)->data), sizeof((pool)->data[0]), index)
#define RB_PoolFree_(pool, index) RB_PoolFreeImpl_(pool, ArrayLength((pool)->data), sizeof((pool)->data[0]), index)

struct RB_Pool_
{ uint32 size, first_free; uint8 data[]; }
typedef RB_Pool_;

static void*
RB_PoolAllocImpl_(void* pool_ptr, uint32 max_size, uintsize obj_size, uint32* out_index)
{
	RB_Pool_* pool = pool_ptr;
	uint32 index = 0;
	
	if (pool->first_free != 0)
	{
		index = pool->first_free;
		
		uint32* next_free_ptr = (uint32*)(pool->data + (index-1) * obj_size);
		pool->first_free = *next_free_ptr;
	}
	else if (pool->size < max_size)
		index = ++pool->size;
	else
		SafeAssert(false);
	
	void* result = pool->data + (index-1) * obj_size;
	Mem_Zero(result, obj_size);
	
	*out_index = index;
	return result;
}

static void*
RB_PoolFetchImpl_(void* pool_ptr, uint32 max_size, uintsize obj_size, uint32 index)
{
	SafeAssert(index != 0);
	SafeAssert(index <= max_size);
	
	RB_Pool_* pool = pool_ptr;
	void* result = pool->data + (index-1) * obj_size;
	
	return result;
}

static void
RB_PoolFreeImpl_(void* pool_ptr, uint32 max_size, uintsize obj_size, uint32 index)
{
	SafeAssert(index != 0);
	SafeAssert(index <= max_size);
	
	RB_Pool_* pool = pool_ptr;
	uint32* next_free_ptr = (uint32*)(pool->data + (index-1) * obj_size);
	
	*next_free_ptr = pool->first_free;
	pool->first_free = index;
}

#ifdef CONFIG_ENABLE_D3D11
#   include "api_os_d3d11.h"
#   include "renderbackend_d3d11.c"
#endif
#ifdef CONFIG_ENABLE_OPENGL
#   include "api_os_opengl.h"
#   include "renderbackend_opengl.c"
#endif

//~
API RB_Ctx*
RB_MakeContext(Arena* arena, const OS_WindowGraphicsContext* graphics_context)
{
	Trace();
	
	RB_Ctx* ctx = Arena_PushStructInit(arena, RB_Ctx, {
		.arena = arena,
		.graphics_context = graphics_context,
	});
	
	switch (graphics_context->api)
	{
		default: break;
#ifdef CONFIG_ENABLE_D3D11
		case OS_WindowGraphicsApi_Direct3D11: RB_SetupD3d11Runtime_(ctx); break;
#endif
#ifdef CONFIG_ENABLE_OPENGL
		case OS_WindowGraphicsApi_OpenGL: RB_SetupOpenGLRuntime_(ctx); break;
#endif
	}
	
	return ctx;
}

API void
RB_FreeContext(RB_Ctx* ctx)
{
	Trace();
	
	Arena* arena = ctx->arena;
	
	ctx->rt_free_ctx(ctx);
	Mem_Zero(ctx, sizeof(RB_Ctx));
	Arena_Pop(arena, ctx);
}

API void
RB_Present(RB_Ctx* ctx)
{
	Trace();
	
	ctx->graphics_context->present_and_vsync(1);
}

API bool
RB_IsValidImpl(RB_Ctx* ctx, uint32 id)
{
	Trace();
	
	return id != 0 && ctx->rt_is_valid_handle(ctx, id);
}

API bool
RB_IsSameImpl(RB_Ctx* ctx, uint32 id1, uint32 id2)
{
	Trace();
	
	return id1 == id2;
}

API RB_Capabilities
RB_QueryCapabilities(RB_Ctx* ctx)
{ return ctx->caps; }

//~
API RB_Tex2d
RB_MakeTexture2D(RB_Ctx* ctx, const RB_Tex2dDesc* desc)
{
	Trace();
	RB_Tex2d handle = { 0 };
	
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_MakeTexture2D_,
		.handle = &handle.id,
		.tex2d = *desc,
	});
	
	return handle;
}

API RB_VBuffer
RB_MakeVertexBuffer(RB_Ctx* ctx, const RB_VBufferDesc* desc)
{
	Trace();
	RB_VBuffer handle = { 0 };
	
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_MakeVertexBuffer_,
		.handle = &handle.id,
		.vbuffer = *desc,
	});
	
	return handle;
}

API RB_IBuffer
RB_MakeIndexBuffer(RB_Ctx* ctx, const RB_IBufferDesc* desc)
{
	Trace();
	RB_IBuffer handle = { 0 };
	
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_MakeIndexBuffer_,
		.handle = &handle.id,
		.ibuffer = *desc,
	});
	
	return handle;
}

API RB_UBuffer
RB_MakeUniformBuffer(RB_Ctx* ctx, const RB_UBufferDesc* desc)
{
	Trace();
	RB_UBuffer handle = { 0 };
	
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_MakeUniformBuffer_,
		.handle = &handle.id,
		.ubuffer = *desc,
	});
	
	return handle;
}

API RB_Shader
RB_MakeShader(RB_Ctx* ctx, const RB_ShaderDesc* desc)
{
	Trace();
	RB_Shader handle = { 0 };
	
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_MakeShader_,
		.handle = &handle.id,
		.shader = *desc,
	});
	
	return handle;
}

API RB_RenderTarget
RB_MakeRenderTarget(RB_Ctx* ctx, const RB_RenderTargetDesc* desc)
{
	Trace();
	RB_RenderTarget handle = { 0 };
	
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_MakeRenderTarget_,
		.handle = &handle.id,
		.render_target = *desc,
	});
	
	return handle;
}

API RB_Pipeline
RB_MakePipeline(RB_Ctx* ctx, const RB_PipelineDesc* desc)
{
	Trace();
	RB_Pipeline handle = { 0 };
	
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_MakePipeline_,
		.handle = &handle.id,
		.pipeline = *desc,
	});
	
	return handle;
}

API void
RB_FreeTexture2D(RB_Ctx* ctx, RB_Tex2d res)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_FreeTexture2D_,
		.handle = &res.id,
	});
}

API void
RB_FreeVertexBuffer(RB_Ctx* ctx, RB_VBuffer res)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_FreeVertexBuffer_,
		.handle = &res.id,
	});
}

API void
RB_FreeIndexBuffer(RB_Ctx* ctx, RB_IBuffer res)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_FreeIndexBuffer_,
		.handle = &res.id,
	});
}

API void
RB_FreeUniformBuffer(RB_Ctx* ctx, RB_UBuffer res)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_FreeUniformBuffer_,
		.handle = &res.id,
	});
}

API void
RB_FreeShader(RB_Ctx* ctx, RB_Shader res)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_FreeShader_,
		.handle = &res.id,
	});
}

API void
RB_FreeRenderTarget(RB_Ctx* ctx, RB_RenderTarget res)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_FreeRenderTarget_,
		.handle = &res.id,
	});
}

API void
RB_FreePipeline(RB_Ctx* ctx, RB_Pipeline res)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_FreePipeline_,
		.handle = &res.id,
	});
}

API void
RB_UpdateVertexBuffer(RB_Ctx* ctx, RB_VBuffer res, Buffer new_data)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_UpdateVertexBuffer_,
		.handle = &res.id,
		.update = {
			.new_data = new_data,
		},
	});
}

API void
RB_UpdateIndexBuffer(RB_Ctx* ctx, RB_IBuffer res, Buffer new_data)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_UpdateIndexBuffer_,
		.handle = &res.id,
		.update = {
			.new_data = new_data,
		},
	});
}

API void
RB_UpdateUniformBuffer(RB_Ctx* ctx, RB_UBuffer res, Buffer new_data)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_UpdateUniformBuffer_,
		.handle = &res.id,
		.update.new_data = new_data,
	});
}

API void
RB_UpdateTexture2D(RB_Ctx* ctx, RB_Tex2d res, Buffer new_data)
{
	Trace();
	ctx->rt_resource(ctx, &(RB_ResourceCall_) {
		.kind = RB_ResourceKind_UpdateTexture2D_,
		.handle = &res.id,
		.update.new_data = new_data,
	});
}

//~
API void
RB_BeginCmd(RB_Ctx* ctx, const RB_BeginDesc* desc)
{
	Trace();
	ctx->rt_cmd(ctx, &(RB_CommandCall_) {
		.kind = RB_CommandKind_Begin_,
		.begin = *desc,
	});
}

API void
RB_CmdApplyPipeline(RB_Ctx* ctx, RB_Pipeline pipeline)
{
	Trace();
	ctx->rt_cmd(ctx, &(RB_CommandCall_) {
		.kind = RB_CommandKind_ApplyPipeline_,
		.apply_pipeline = { pipeline },
	});
}

API void
RB_CmdApplyRenderTarget(RB_Ctx* ctx, RB_RenderTarget render_target)
{
	Trace();
	ctx->rt_cmd(ctx, &(RB_CommandCall_) {
		.kind = RB_CommandKind_ApplyRenderTarget_,
		.apply_render_target = { render_target },
	});
}

API void
RB_CmdClear(RB_Ctx* ctx, const RB_ClearDesc* desc)
{
	Trace();
	ctx->rt_cmd(ctx, &(RB_CommandCall_) {
		.kind = RB_CommandKind_Clear_,
		.clear = *desc,
	});
}

API void
RB_CmdDraw(RB_Ctx* ctx, const RB_DrawDesc* desc)
{
	Trace();
	ctx->rt_cmd(ctx, &(RB_CommandCall_) {
		.kind = RB_CommandKind_Draw_,
		.draw = *desc,
	});
}

API void
RB_EndCmd(RB_Ctx* ctx)
{
	Trace();
	ctx->rt_cmd(ctx, &(RB_CommandCall_) {
		.kind = RB_CommandKind_End_,
	});
}
