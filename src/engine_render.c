uint8 typedef BYTE;

#include "engine_internal_d3d11_vshader_quad.inc"
#include "engine_internal_d3d11_pshader_quad.inc"

static RB_Handle g_render_quadvbuf;
static RB_Handle g_render_quadibuf;
static RB_Handle g_render_whitetex;
static RB_Handle g_render_quadshader;
static RB_Handle g_render_quadelemsbuf;

struct E_Tex2d
{
	RB_Handle handle;
};

static const char g_render_gl_quadvshader[] =
"#version 330 core\n"
"layout (location=0) in vec2  aPos;\n"
"layout (location=1) in vec2  aElemPos;\n"
"layout (location=2) in mat2  aElemScaling;\n"
"layout (location=4) in float aElemTexIndex;\n"
"layout (location=5) in vec4  aElemTexcoords;\n"
"layout (location=6) in vec4  aElemColor;\n"
"\n"
"out vec2  vTexcoords;\n"
"out vec4  vColor;\n"
"out float vTexIndex;\n"
"\n"
"uniform UniformBuffer {\n"
"    mat4 uView;\n"
"};\n"
"\n"
"void main() {\n"
"    gl_Position = uView * vec4(aElemPos + aElemScaling * aPos, 0.0, 1.0);\n"
"    vTexcoords = aElemTexcoords.xy + aElemTexcoords.zw * aPos;\n"
"    vColor = aElemColor;\n"
"    vTexIndex = aElemTexIndex;\n"
"}\n"
"\n";

static const char g_render_gl_quadfshader[] =
"#version 330 core\n"
"in vec2  vTexcoords;\n"
"in vec4  vColor;\n"
"in float vTexIndex;\n"
"\n"
"out vec4 oFragColor;\n"
"\n"
"uniform sampler2D uTexture[8];\n"
"\n"
"void main() {\n"
"    vec4 color = vec4(1.0);\n"
"    \n"
"    switch (int(vTexIndex)) {\n"
"        default:\n"
"        case 0: color = texture(uTexture[0], vTexcoords); break;\n"
"        case 1: color = texture(uTexture[1], vTexcoords); break;\n"
"        case 2: color = texture(uTexture[2], vTexcoords); break;\n"
"        case 3: color = texture(uTexture[3], vTexcoords); break;\n"
"        case 4: color = texture(uTexture[4], vTexcoords); break;\n"
"        case 5: color = texture(uTexture[5], vTexcoords); break;\n"
"        case 6: color = texture(uTexture[6], vTexcoords); break;\n"
"        case 7: color = texture(uTexture[7], vTexcoords); break;\n"
"    }\n"
"    \n"
"    oFragColor = color * vColor;\n"
"}\n"
"\n";

static void
E_AppendDrawCmd_(RB_DrawCommand* first, RB_DrawCommand* last)
{
	if (!global_engine.last_draw_command)
	{
		global_engine.draw_command_list = first;
		global_engine.last_draw_command = last;
	}
	else
	{
		global_engine.last_draw_command->next = first;
		global_engine.last_draw_command = last;
	}
}

static void
E_InitRender_(void)
{
	Trace();
	
	Arena* arena = global_engine.scratch_arena;
	RB_Init(arena, global_engine.graphics_context);
	
	float32 quadvbuf[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
	};
	
	uint32 quadibuf[] = {
		0, 1, 2,
		2, 3, 1,
	};
	
	uint32 whitetex[] = {
		0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF,
	};
	
	for Arena_TempScope(arena)
	{
		RB_ResourceCommand* list = NULL;
		RB_ResourceCommand** head = &list;
		RB_ResourceCommand* cmd = NULL;
		
		// Quad vertex buffer
		cmd = Arena_PushStruct(arena, RB_ResourceCommand);
		*cmd = (RB_ResourceCommand) {
			.kind = RB_ResourceCommandKind_MakeVertexBuffer,
			.handle = &g_render_quadvbuf,
			.buffer = {
				.memory = quadvbuf,
				.size = sizeof(quadvbuf),
			},
		};
		
		*head = cmd;
		head = &cmd->next;
		
		// Quad index buffer
		cmd = Arena_PushStruct(arena, RB_ResourceCommand);
		*cmd = (RB_ResourceCommand) {
			.kind = RB_ResourceCommandKind_MakeIndexBuffer,
			.handle = &g_render_quadibuf,
			.buffer = {
				.memory = quadibuf,
				.size = sizeof(quadibuf),
			},
		};
		
		*head = cmd;
		head = &cmd->next;
		
		// White texture
		cmd = Arena_PushStruct(arena, RB_ResourceCommand);
		*cmd = (RB_ResourceCommand) {
			.kind = RB_ResourceCommandKind_MakeTexture2D,
			.handle = &g_render_whitetex,
			.texture_2d = {
				.pixels = whitetex,
				.width = 2,
				.height = 2,
				.channels = 4,
			},
		};
		
		*head = cmd;
		head = &cmd->next;
		
		// Quad elements vertex buffer
		cmd = Arena_PushStruct(arena, RB_ResourceCommand);
		*cmd = (RB_ResourceCommand) {
			.kind = RB_ResourceCommandKind_MakeVertexBuffer,
			.handle = &g_render_quadelemsbuf,
			.flag_dynamic = true,
			.buffer = {
				.memory = NULL,
				.size = sizeof(E_RectBatchElem)*1024,
			},
		};
		
		*head = cmd;
		head = &cmd->next;
		
		// Quad shader
		cmd = Arena_PushStruct(arena, RB_ResourceCommand);
		*cmd = (RB_ResourceCommand) {
			.kind = RB_ResourceCommandKind_MakeShader,
			.handle = &g_render_quadshader,
			.shader = {
				.d3d_vs_blob = Buf(g_D3d11Shader_QuadVertex),
				.d3d_ps_blob = Buf(g_D3d11Shader_QuadPixel),
				.gl_vs_src = StrInit(g_render_gl_quadvshader),
				.gl_fs_src = StrInit(g_render_gl_quadfshader),
				
				.input_layout = {
					[0] = {
						.kind = RB_LayoutDescKind_Vec2,
						.offset = 0,
						.divisor = 0,
						.vbuffer_index = 0,
						.gl_location = 0,
					},
					[1] = {
						.kind = RB_LayoutDescKind_Vec2,
						.offset = offsetof(E_RectBatchElem, pos),
						.divisor = 1,
						.vbuffer_index = 1,
						.gl_location = 1,
					},
					[2] = {
						.kind = RB_LayoutDescKind_Mat2,
						.offset = offsetof(E_RectBatchElem, scaling),
						.divisor = 1,
						.vbuffer_index = 1,
						.gl_location = 2,
					},
					[3] = {
						.kind = RB_LayoutDescKind_Float,
						.offset = offsetof(E_RectBatchElem, tex_index),
						.divisor = 1,
						.vbuffer_index = 1,
						.gl_location = 4,
					},
					[4] = {
						.kind = RB_LayoutDescKind_Vec4,
						.offset = offsetof(E_RectBatchElem, texcoords),
						.divisor = 1,
						.vbuffer_index = 1,
						.gl_location = 5,
					},
					[5] = {
						.kind = RB_LayoutDescKind_Vec4,
						.offset = offsetof(E_RectBatchElem, color),
						.divisor = 1,
						.vbuffer_index = 1,
						.gl_location = 6,
					},
				},
			},
		};
		
		*head = cmd;
		head = &cmd->next;
		
		// Run!
		RB_ExecuteResourceCommands(arena, list);
	}
}

static void
E_DeinitRender_(void)
{
	Trace();
	
	RB_Deinit(global_engine.scratch_arena);
}

//- Helpers
API void
E_CalcViewMatrix2D(const E_Camera2D* camera, mat4 out_view)
{
	vec3 size = {
		camera->zoom * 2.0f / camera->size[0],
		-camera->zoom * 2.0f / camera->size[1],
		1.0f,
	};
	
	mat4 view;
	glm_mat4_identity(view);
	glm_translate(view, (vec3) { -camera->pos[0] * size[0], -camera->pos[1] * size[1] });
	glm_rotate(view, camera->angle, (vec3) { 0.0f, 0.0f, 1.0f });
	glm_scale(view, size);
	glm_translate(view, (vec3) { -0.5f, -0.5f });
	glm_mat4_copy(view, out_view);
}

API void
E_CalcViewMatrix3D(const E_Camera3D* camera, mat4 out_view, float32 fov, float32 aspect)
{
	glm_mat4_identity(out_view);
	glm_look((float32*)camera->pos, (float32*)camera->dir, (float32*)camera->up, out_view);
	
	mat4 proj;
	glm_perspective(fov, aspect, 0.01f, 100.0f, proj);
	glm_mat4_mul(proj, out_view, out_view);
}

API void
E_CalcModelMatrix2D(const vec2 pos, const vec2 scale, float32 angle, mat4 out_model)
{
	mat4 model;
	glm_mat4_identity(model);
	glm_translate(model, (vec3) { pos[0], pos[1] });
	glm_scale(model, (vec3) { scale[0], scale[1], 1.0f });
	glm_rotate(model, angle, (vec3) { 0.0f, 0.0f, 1.0f });
	glm_mat4_copy(model, out_model);
}

API void
E_CalcModelMatrix3D(const vec3 pos, const vec3 scale, const vec3 rot, mat4 out_model)
{
	mat4 model;
	glm_mat4_identity(model);
	glm_translate(model, (float32*)pos);
	glm_scale(model, (float32*)scale);
	glm_rotate(model, rot[0], (vec3) { 1.0f, 0.0f, 0.0f });
	glm_rotate(model, rot[1], (vec3) { 0.0f, 1.0f, 0.0f });
	glm_rotate(model, rot[2], (vec3) { 0.0f, 0.0f, 1.0f });
	glm_mat4_copy(model, out_model);
}

API void
E_CalcPointInCamera2DSpace(const E_Camera2D* camera, const vec2 pos, vec2 out_pos)
{
	vec2 result;
	float32 inv_zoom = 1.0f / camera->zoom * 2.0f;
	
	result[0] = camera->pos[0] + (pos[0] - camera->size[0] * 0.5f) * inv_zoom;
	result[1] = camera->pos[1] + (pos[1] - camera->size[1] * 0.5f) * inv_zoom;
	//result[1] *= -1.0f;
	
	glm_vec2_copy(result, out_pos);
}

//- New api
API void
E_DrawClear(float32 r, float32 g, float32 b, float32 a)
{
	Trace();
	
	RB_DrawCommand* cmd = Arena_PushStruct(global_engine.frame_arena, RB_DrawCommand);
	
	cmd->kind = RB_DrawCommandKind_Clear;
	cmd->clear.flag_color = true;
	cmd->clear.color[0] = r;
	cmd->clear.color[1] = g;
	cmd->clear.color[2] = b;
	cmd->clear.color[3] = a;
	
	E_AppendDrawCmd_(cmd, cmd);
}

API void
E_DrawRectBatch(const E_RectBatch* batch)
{
	Trace();
	
	mat4 view;
	E_Camera2D cam = {
		.pos = { 0.0f, 0.0f },
		.size = { (float32)global_engine.window_state->width, (float32)global_engine.window_state->height },
		.zoom = 1.0f,
		.angle = 0.0f,
	};
	
	E_CalcViewMatrix2D(&cam, view);
	
	Buffer uniform_buffer = Arena_PushStringAligned(global_engine.frame_arena, Buf(view), 16);
	
	RB_ResourceCommand* rc_cmd = Arena_PushStruct(global_engine.frame_arena, RB_ResourceCommand);
	*rc_cmd = (RB_ResourceCommand) {
		.kind = RB_ResourceCommandKind_UpdateVertexBuffer,
		.handle = &g_render_quadelemsbuf,
		.buffer = {
			.memory = batch->elements,
			.size = batch->count * sizeof(batch->elements[0]),
		},
	};
	
	RB_DrawCommand* cmd = Arena_PushStruct(global_engine.frame_arena, RB_DrawCommand);
	*cmd = (RB_DrawCommand) {
		.kind = RB_DrawCommandKind_DrawCall,
		.resources_cmd = rc_cmd,
		.drawcall = {
			.shader = &g_render_quadshader,
			.ibuffer = &g_render_quadibuf,
			.vbuffers = { &g_render_quadvbuf, &g_render_quadelemsbuf, },
			.vbuffer_strides = { sizeof(vec2), sizeof(E_RectBatchElem), },
			.index_count = 6,
			.instance_count = batch->count,
			.uniform_buffer = uniform_buffer,
			.samplers = {
				{ batch->textures[0] ? &batch->textures[0]->handle : &g_render_whitetex, },
				{ batch->textures[1] ? &batch->textures[1]->handle : &g_render_whitetex, },
				{ batch->textures[2] ? &batch->textures[2]->handle : &g_render_whitetex, },
				{ batch->textures[3] ? &batch->textures[3]->handle : &g_render_whitetex, },
				{ batch->textures[4] ? &batch->textures[4]->handle : &g_render_whitetex, },
				{ batch->textures[5] ? &batch->textures[5]->handle : &g_render_whitetex, },
				{ batch->textures[6] ? &batch->textures[6]->handle : &g_render_whitetex, },
				{ batch->textures[7] ? &batch->textures[7]->handle : &g_render_whitetex, },
			},
		},
	};
	
	E_AppendDrawCmd_(cmd, cmd);
}
