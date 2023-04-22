
uint8 typedef BYTE;

#include <d3d11_shader_quad_vs.inc>
#include <d3d11_shader_quad_ps.inc>
#include <d3d11_shader_quad_level91_vs.inc>
#include <d3d11_shader_quad_level91_ps.inc>

static RB_VBuffer g_render_quadvbuf;
static RB_IBuffer g_render_quadibuf;
static RB_UBuffer g_render_quadubuf;
static RB_Tex2d g_render_whitetex;
static RB_Shader g_render_quadshader;
static RB_Pipeline g_render_quadpipeline;
static RB_VBuffer g_render_quadelemsbuf;
static intsize g_render_uninstanced_cached_count = 1;
static RB_Capabilities g_render_caps;

static const char g_render_gl_quadvshader[] =
"layout (location=0) in vec2  aPos;\n"
"layout (location=1) in vec2  aElemPos;\n"
"layout (location=2) in mat2  aElemScaling;\n"
"layout (location=4) in vec2  aElemTexIndex;\n"
"layout (location=5) in vec4  aElemTexcoords;\n"
"layout (location=6) in vec4  aElemColor;\n"
"\n"
"out vec2  vTexcoords;\n"
"out vec4  vColor;\n"
"out vec2  vTexIndex;\n"
"out vec2  vRawPos;\n"
"out vec2  vRawScale;\n"
"\n"
"layout(std140) uniform UniformBuffer {\n"
"    mat4 uView;\n"
"    vec4 uTexsize01;\n"
"    vec4 uTexsize23;\n"
"};\n"
"\n"
"void main() {\n"
"    gl_Position = uView * vec4(aElemPos + aElemScaling * aPos, 0.0, 1.0);\n"
"    vTexcoords = aElemTexcoords.xy + aElemTexcoords.zw * aPos;\n"
"    vColor = aElemColor;\n"
"    vTexIndex = aElemTexIndex;\n"
"    vRawPos = aPos;\n"
"    vRawScale = vec2(length(aElemScaling[0]), length(aElemScaling[1]));\n"
"}\n"
"\n";

static const char g_render_gl_quadfshader[] =
"in vec2  vTexcoords;\n"
"in vec4  vColor;\n"
"in vec2  vTexIndex;\n"
"in vec2  vRawPos;\n"
"in vec2  vRawScale;\n"
"\n"
"out vec4 oFragColor;\n"
"\n"
"layout(std140) uniform UniformBuffer {\n"
"    mat4 uView;\n"
"    vec4 uTexsize01;\n"
"    vec4 uTexsize23;\n"
"};\n"
"\n"
"uniform sampler2D uTexture[4];\n"
"\n"
"void main() {\n"
"    vec4 color = vec4(1.0);\n"
"    vec2 texsize;\n"
"    \n"
"    switch (int(vTexIndex.x)) {\n"
"        default:\n"
"        case 0: color = texture(uTexture[0], vTexcoords); texsize = uTexsize01.xy; break;\n"
"        case 1: color = texture(uTexture[1], vTexcoords); texsize = uTexsize01.zw; break;\n"
"        case 2: color = texture(uTexture[2], vTexcoords); texsize = uTexsize23.xy; break;\n"
"        case 3: color = texture(uTexture[3], vTexcoords); texsize = uTexsize23.zw; break;\n"
"    }\n"
"    \n"
"    switch (int(vTexIndex.y)) {\n"
"        default:"
"        case 0: break;"
"        case 1: color = vec4(1.0, 1.0, 1.0, color.x); break;"
"        case 2: {\n"
"            vec2 pos = (vRawPos - 0.5) * 2.0;\n"
"            if (dot(pos, pos) >= 1.0)\n"
"                color.w = 0.0;\n"
"        } break;\n"
"        case 3: {\n"
"            // Derived from: https://www.shadertoy.com/view/4llXD7\n"
"            float r = 0.5 * min(vRawScale.x, vRawScale.y);\n"
"            vec2 p = (vRawPos - 0.5) * 2.0 * vRawScale;\n"
"            vec2 q = abs(p) - vRawScale + r;\n"
"            float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;\n"
"            \n"
"            if (dist >= -1.0)\n"
"                color.w *= max(1.0 - (dist+1.0)*0.5, 0.0);\n"
"        } break;\n"
"        case 4: {\n"
"            vec2 density = fwidth(vTexcoords) * texsize;\n"
"            float m = min(density.x, density.y);\n"
"            float inv = 1.0 / m;\n"
"            float a = (color.x - 128.0/255.0 + 24.0/255.0*m*0.5) * 255.0/24.0 * inv;\n"
"            color = vec4(1.0, 1.0, 1.0, a);\n"
"        } break;\n"
"    }\n"
"    \n"
"    oFragColor = color * vColor;\n"
"}\n"
"\n";

static void
E_InitRender_(void)
{
	Trace();
	
	RB_Ctx* renderbackend = RB_MakeContext(global_engine.persistent_arena, global_engine.os->graphics_context);
	global_engine.renderbackend = renderbackend;
	
	// NOTE(ljre): Print capabilities
	{
		RB_Capabilities cap = RB_QueryCapabilities(renderbackend);
		g_render_caps = cap;
		
		OS_DebugLog("[RB] backend: %S\n[RB] driver renderer: %S\n[RB] driver vendor: %S\n[RB] driver version: %S\n", cap.backend_api, cap.driver_renderer, cap.driver_vendor, cap.driver_version);
	}
	
	int16 quadvbuf[] = {
		0, 0,
		0, INT16_MAX,
		INT16_MAX, 0,
		INT16_MAX, INT16_MAX,
	};
	
	uint16 quadibuf[] = {
		0, 1, 2,
		2, 1, 3,
	};
	
	uint32 whitetex[] = {
		0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFF, 0xFFFFFFFF,
	};
	
	g_render_quadvbuf = RB_MakeVertexBuffer(renderbackend, &(RB_VBufferDesc) {
		.flag_dynamic = !g_render_caps.has_instancing,
		.size = sizeof(quadvbuf),
		.initial_data = quadvbuf,
	});
	
	g_render_quadibuf = RB_MakeIndexBuffer(renderbackend, &(RB_IBufferDesc) {
		.flag_dynamic = !g_render_caps.has_instancing,
		.size = sizeof(quadibuf),
		.initial_data = quadibuf,
		.index_type = RB_IndexType_Uint16,
	});
	
	g_render_whitetex = RB_MakeTexture2D(renderbackend, &(RB_Tex2dDesc) {
		.pixels = whitetex,
		.width = 2,
		.height = 2,
		.format = RB_TexFormat_RGBA8,
	});
	
	g_render_quadelemsbuf = RB_MakeVertexBuffer(renderbackend, &(RB_VBufferDesc) {
		.flag_dynamic = true,
		.size = sizeof(E_RectBatchElem)*1024,
	});
	
	g_render_quadubuf = RB_MakeUniformBuffer(renderbackend, &(RB_UBufferDesc) {
		.size = sizeof(mat4)+sizeof(vec2[RB_Limits_DrawMaxTextures]),
	});
	
	g_render_quadshader = RB_MakeShader(renderbackend, &(RB_ShaderDesc) {
		.hlsl40 = {
			BufInit(g_render_d3d11_shader_quad_vs),
			BufInit(g_render_d3d11_shader_quad_ps)
		},
		.hlsl40_91 = {
			BufInit(g_render_d3d11_shader_quad_level91_vs),
			BufInit(g_render_d3d11_shader_quad_level91_ps),
		},
		.glsl = {
			StrInit(g_render_gl_quadvshader),
			StrInit(g_render_gl_quadfshader),
		},
	});
	
	g_render_quadpipeline = RB_MakePipeline(renderbackend, &(RB_PipelineDesc) {
		.flag_blend = true,
		
		.blend_source = RB_BlendFunc_SrcAlpha,
		.blend_dest = RB_BlendFunc_InvSrcAlpha,
		.blend_op = RB_BlendOp_Add,
		.blend_source_alpha = RB_BlendFunc_SrcAlpha,
		.blend_dest_alpha = RB_BlendFunc_InvSrcAlpha,
		.blend_op_alpha = RB_BlendOp_Add,
		
		.shader = g_render_quadshader,
		.input_layout = {
			[0] = {
				.format = RB_VertexFormat_Vec2I16Norm,
				.buffer_slot = 0,
			},
			[1] = {
				.format = RB_VertexFormat_Vec2,
				.offset = offsetof(E_RectBatchElem, pos),
				.divisor = g_render_caps.has_instancing,
				.buffer_slot = 1,
			},
			[2] = {
				.format = RB_VertexFormat_Mat2,
				.offset = offsetof(E_RectBatchElem, scaling),
				.divisor = g_render_caps.has_instancing,
				.buffer_slot = 1,
			},
			[3] = {
				.format = RB_VertexFormat_Vec2I16,
				.offset = offsetof(E_RectBatchElem, tex_index),
				.divisor = g_render_caps.has_instancing,
				.buffer_slot = 1,
			},
			[4] = {
				.format = RB_VertexFormat_Vec4I16Norm,
				.offset = offsetof(E_RectBatchElem, texcoords),
				.divisor = g_render_caps.has_instancing,
				.buffer_slot = 1,
			},
			[5] = {
				.format = RB_VertexFormat_Vec4,
				.offset = offsetof(E_RectBatchElem, color),
				.divisor = g_render_caps.has_instancing,
				.buffer_slot = 1,
			},
		},
	});
	
	RB_BeginCmd(global_engine.renderbackend, &(RB_BeginDesc) {
		.viewport_width = global_engine.os->window.width,
		.viewport_height = global_engine.os->window.height,
	});
}

static void
E_DeinitRender_(void)
{
	Trace();
	
	RB_FreeContext(global_engine.renderbackend);
	global_engine.renderbackend = NULL;
}

//- Helpers
API void
E_CalcViewMatrix2D(const E_Camera2D* camera, mat4 out_view)
{
	Trace();
	
	vec3 size = {
		camera->zoom * 2.0f / camera->size[0],
		-camera->zoom * 2.0f / camera->size[1],
		1.0f,
	};
	
	mat4 view;
	glm_mat4_identity(view);
	glm_translate(view, vec3(-camera->pos[0] * size[0] - 1.0f, -camera->pos[1] * size[1] + 1.0f));
	glm_rotate(view, camera->angle, vec3(0.0f, 0.0f, 1.0f));
	glm_scale(view, size);
	//glm_translate(view, vec3(-0.5f, -0.5f));
	glm_mat4_copy(view, out_view);
}

API void
E_CalcViewMatrix3D(const E_Camera3D* camera, mat4 out_view, float32 fov, float32 aspect)
{
	Trace();
	
	glm_mat4_identity(out_view);
	glm_look((float32*)camera->pos, (float32*)camera->dir, (float32*)camera->up, out_view);
	
	mat4 proj;
	glm_perspective(fov, aspect, 0.01f, 100.0f, proj);
	glm_mat4_mul(proj, out_view, out_view);
}

API void
E_CalcModelMatrix2D(const vec2 pos, const vec2 scale, float32 angle, mat4 out_model)
{
	Trace();
	
	mat4 model;
	glm_mat4_identity(model);
	glm_translate(model, vec3(pos[0], pos[1]));
	glm_scale(model, vec3(scale[0], scale[1], 1.0f));
	glm_rotate(model, angle, vec3(0.0f, 0.0f, 1.0f));
	glm_mat4_copy(model, out_model);
}

API void
E_CalcModelMatrix3D(const vec3 pos, const vec3 scale, const vec3 rot, mat4 out_model)
{
	Trace();
	
	mat4 model;
	glm_mat4_identity(model);
	glm_translate(model, (float32*)pos);
	glm_scale(model, (float32*)scale);
	glm_rotate(model, rot[0], vec3(1.0f, 0.0f, 0.0f));
	glm_rotate(model, rot[1], vec3(0.0f, 1.0f, 0.0f));
	glm_rotate(model, rot[2], vec3(0.0f, 0.0f, 1.0f));
	glm_mat4_copy(model, out_model);
}

API void
E_CalcPointInCamera2DSpace(const E_Camera2D* camera, const vec2 pos, vec2 out_pos)
{
	Trace();
	
	vec2 result;
	float32 inv_zoom = 1.0f / camera->zoom * 2.0f;
	
	result[0] = camera->pos[0] + (pos[0] - camera->size[0] * 0.5f) * inv_zoom;
	result[1] = camera->pos[1] + (pos[1] - camera->size[1] * 0.5f) * inv_zoom;
	//result[1] *= -1.0f;
	
	glm_vec2_copy(result, out_pos);
}

//- New api
API E_Tex2d
E_WhiteTexture(void)
{
	return (E_Tex2d) { g_render_whitetex, 2, 2 };
}

API bool
E_MakeTex2d(const E_Tex2dDesc* desc, E_Tex2d* out_tex)
{
	Trace();
	
	const void* pixels;
	int32 width, height;
	RB_TexFormat format;
	bool needs_to_call_stbi_image_free = false;
	
	if (desc->encoded_image.size)
	{
		SafeAssert(desc->encoded_image.size <= INT32_MAX);
		
		const void* data = desc->encoded_image.data;
		int32 size = (int32)desc->encoded_image.size;
		
		pixels = stbi_load_from_memory(data, size, &width, &height, &(int32) { 0 }, 4);
		format = RB_TexFormat_RGBA8;
		needs_to_call_stbi_image_free = true;
		
		if (!pixels)
			return false;
	}
	else
	{
		const void* data = desc->raw_image.data;
		uintsize size = desc->raw_image.size;
		
		pixels = data;
		width = desc->width;
		height = desc->height;
		format = desc->raw_image_format;
		
		SafeAssert(data && size);
		//SafeAssert(size == (intsize)width*height*channels);
	}
	
	// Generate command
	*out_tex = (E_Tex2d) {
		.handle = RB_MakeTexture2D(global_engine.renderbackend, &(RB_Tex2dDesc) {
			.pixels = pixels,
			.width = width,
			.height = height,
			.format = format,
		}),
		.width = width,
		.height = height,
	};
	
	if (needs_to_call_stbi_image_free)
		stbi_image_free((void*)pixels);
	
	return true;
}

API bool
E_MakeFont(const E_FontDesc* desc, E_Font* out_font)
{
	Trace();
	
	stbtt_fontinfo* stb_fontinfo = Arena_PushStruct(desc->arena, stbtt_fontinfo);
	Buffer ttf = desc->ttf;
	
	{
		Trace(); TraceName(Str("stbtt_InitFont"));
		if (!stbtt_InitFont(stb_fontinfo, ttf.data, 0))
			return false;
	}
	
	stb_fontinfo->userdata = global_engine.scratch_arena;
	
	int32 tex_size = desc->bitmap_size ? desc->bitmap_size : 512;
	uint8* bitmap;
	
	{
		Trace(); TraceName(Str("Allocate bitmap"));
		bitmap = Arena_PushAligned(desc->arena, tex_size*tex_size, 4);
	}
	
	uint32 glyphmap_count = 0;
	uint32 glyphmap_log2cap = desc->hashmap_log2cap ? desc->hashmap_log2cap : 16;
	E_FontGlyphEntry invalid_glyph = { 0 };
	E_FontGlyphEntry* glyphmap;
	
	{
		Trace(); TraceName(Str("Allocate hashmap"));
		glyphmap = Arena_PushAligned(desc->arena, sizeof(*glyphmap) << glyphmap_log2cap, 4);
	}
	
	int32 ascent;
	int32 descent;
	int32 line_gap;
	int32 space_advance;
	int32 bbox_y1;
	int32 bbox_y2;
	float32 char_scale;
	
	{
		Trace(); TraceName(Str("stbtt_ScaleForPixelHeight"));
		char_scale = stbtt_ScaleForPixelHeight(stb_fontinfo, (desc->char_height != 0) ? desc->char_height : 16.0f);
	}
	
	{
		Trace(); TraceName(Str("stbtt_GetFontVMetrics"));
		stbtt_GetFontVMetrics(stb_fontinfo, &ascent, &descent, &line_gap);
	}
	{
		Trace(); TraceName(Str("stbtt_GetCodepointHMetrics"));
		stbtt_GetCodepointHMetrics(stb_fontinfo, ' ', &space_advance, &(int32) { 0 });
	}
	{
		Trace(); TraceName(Str("stbtt_GetFontBoundingBox"));
		stbtt_GetFontBoundingBox(stb_fontinfo, &(int32) { 0 }, &bbox_y1, &(int32) { 0 }, &bbox_y2);
	}
	
	int32 tex_currline = 0;
	int32 tex_currcol = 0;
	int32 tex_linesize = (int32)ceilf((bbox_y2 - bbox_y1) * char_scale + 0.5f);
	
	for (intsize i = -1; i < ArrayLength(desc->prebake_ranges); ++i)
	{
		uint32 range_begin;
		uint32 range_end;
		
		// NOTE(ljre): Handle special case of the replacement character.
		if (i == -1)
			range_end = range_begin = 0xFFFD;
		else
		{
			if (!desc->prebake_ranges[i].begin)
				break;
			
			range_begin = Max(' '+1, desc->prebake_ranges[i].begin);
			range_end = desc->prebake_ranges[i].end;
		}
		
		for (uint32 codepoint = range_begin; codepoint <= range_end; ++codepoint)
		{
			E_FontGlyphEntry* glyph = NULL;
			
			if (i == -1)
				glyph = &invalid_glyph;
			else
			{
				uint64 hash = Hash_IntHash64(codepoint);
				int32 index = (int32)hash;
				
				for (;;)
				{
					index = Hash_Msi(glyphmap_log2cap, hash, index);
					glyph = &glyphmap[index];
					
					if (glyph->codepoint == codepoint)
					{
						glyph = NULL;
						break;
					}
					
					if (!glyph->codepoint)
					{
						++glyphmap_count;
						SafeAssert(glyphmap_count <= (1u << glyphmap_log2cap));
						break;
					}
				}
			}
			
			if (!glyph)
				continue;
			
			int32 glyph_font_index;
			int32 advance, bearing;
			int32 x1, y1, x2, y2;
			
			glyph_font_index = stbtt_FindGlyphIndex(stb_fontinfo, (int32)codepoint);
			stbtt_GetGlyphHMetrics(stb_fontinfo, glyph_font_index, &advance, &bearing);
			stbtt_GetGlyphBitmapBox(stb_fontinfo, glyph_font_index, char_scale, char_scale, &x1, &y1, &x2, &y2);
			
			// NOTE(ljre): Extra padding -- giving more space to the SDF
			x1 -= 4;
			y1 -= 4;
			x2 += 4;
			y2 += 4;
			
			int32 width = x2 - x1;
			int32 height = y2 - y1;
			
			if (tex_currcol + width >= tex_size)
			{
				tex_currcol = 0;
				tex_currline += 1;
				
				SafeAssert((tex_currline - 1) * tex_linesize < tex_size);
			}
			
			int32 x = tex_currcol;
			int32 y = tex_currline * tex_linesize;
			
			tex_currcol += width + 1;
			
			*glyph = (E_FontGlyphEntry) {
				.codepoint = codepoint,
				.x = (uint16)x,
				.y = (uint16)y,
				.width = (uint16)width,
				.height = (uint16)height,
				.xoff = (int16)x1,
				.yoff = (int16)y1,
				.advance = (int16)advance,
				.bearing = (int16)bearing,
			};
			
			uint8* base_ptr = bitmap + (x + y * tex_size);
			int32 stride = tex_size;
			
			// NOTE(ljre): stbtt_GetGlyphSDF will alloc in the scratch arena.
			for Arena_TempScope(global_engine.scratch_arena)
			{
				int32 xoff, yoff, w, h;
				uint8* sdf;
				
				{
					Trace(); TraceName(Str("stbtt_GetGlyphSDF"));
					sdf = stbtt_GetGlyphSDF(
						stb_fontinfo, char_scale, glyph_font_index, 4, 128, 24.0f,
						&w, &h, &xoff, &yoff);
				}
				
				SafeAssert(w == width && h == height);
				
				for (intsize y = 0; y < height; ++y)
					Mem_Copy(base_ptr + y * stride, sdf + y * w, width);
			}
		}
	}
	
	*out_font = (E_Font) {
		.texture = {
			.handle = RB_MakeTexture2D(global_engine.renderbackend, &(RB_Tex2dDesc) {
				.pixels = bitmap,
				.width = tex_size,
				.height = tex_size,
				.format = RB_TexFormat_R8,
				.flag_linear_filtering = true,
			}),
			.width = tex_size,
			.height = tex_size,
		},
		
		.ttf = ttf,
		.stb_fontinfo = stb_fontinfo,
		
		.bitmap = bitmap,
		.tex_size = tex_size,
		.tex_currline = tex_currline,
		.tex_currcol = tex_currcol,
		.tex_linesize = tex_linesize,
		
		.glyphmap_count = glyphmap_count,
		.glyphmap_log2cap = glyphmap_log2cap,
		.glyphmap = glyphmap,
		.invalid_glyph = invalid_glyph,
		
		.ascent = ascent,
		.descent = descent,
		.line_gap = line_gap,
		.space_advance = space_advance,
		.char_scale = char_scale,
	};
	
	return true;
}

API void
E_DrawRectBatch(const E_RectBatch* batch, const E_Camera2D* cam)
{
	Trace();
	RB_Ctx* rb = global_engine.renderbackend;
	
	struct
	{
		mat4 view;
		vec2 texsize[RB_Limits_DrawMaxTextures];
	}
	ubuffer = { 0 };
	
	if (!cam)
	{
		cam = &(E_Camera2D) {
			.pos = { 0.0f, 0.0f },
			.size = { (float32)global_engine.os->window.width, (float32)global_engine.os->window.height },
			.zoom = 1.0f,
			.angle = 0.0f,
		};
	}
	
	E_CalcViewMatrix2D(cam, ubuffer.view);
	
	for (intsize i = 0; i < ArrayLength(batch->textures); ++i)
	{
		if (!RB_IsNull(batch->textures[i].handle))
		{
			ubuffer.texsize[i][0] = batch->textures[i].width;
			ubuffer.texsize[i][1] = batch->textures[i].height;
		}
		else
		{
			ubuffer.texsize[i][0] = 2.0f;
			ubuffer.texsize[i][1] = 2.0f;
		}
	}
	
	RB_UpdateUniformBuffer(rb, g_render_quadubuf, BufMake(sizeof(ubuffer), &ubuffer));
	
	if (g_render_caps.has_instancing)
	{
		uintsize size = batch->count * sizeof(batch->elements[0]);
		RB_UpdateVertexBuffer(rb, g_render_quadelemsbuf, BufMake(size, batch->elements));
	}
	else
	{
		for Arena_TempScope(global_engine.scratch_arena)
		{
			uintsize elements_size = sizeof(E_RectBatchElem) * batch->count * 4;
			E_RectBatchElem* elements = Arena_PushDirty(global_engine.scratch_arena, elements_size);
			
			for (intsize i = 0; i < batch->count; ++i)
			{
				elements[i*4 + 0] = batch->elements[i];
				elements[i*4 + 1] = batch->elements[i];
				elements[i*4 + 2] = batch->elements[i];
				elements[i*4 + 3] = batch->elements[i];
			}
			
			RB_UpdateVertexBuffer(rb, g_render_quadelemsbuf, BufMake(elements_size, elements));
		}
		
		if (g_render_uninstanced_cached_count < batch->count)
		{
			for Arena_TempScope(global_engine.scratch_arena)
			{
				g_render_uninstanced_cached_count = batch->count;
				
				uintsize new_vertices_size = sizeof(int16) * 8 * batch->count;
				uintsize new_indices_size = sizeof(uint16) * 6 * batch->count;
				
				int16* new_vertices = Arena_PushDirty(global_engine.scratch_arena, new_vertices_size);
				uint16* new_indices = Arena_PushDirty(global_engine.scratch_arena, new_indices_size);
				
				alignas(16) int16 base_vertices[8] = {
					0, 0,
					0, INT16_MAX,
					INT16_MAX, 0,
					INT16_MAX, INT16_MAX,
				};
				
				static_assert(sizeof(base_vertices) == 16, "making sure we can copy a whole vertex at a time");
				
				for (intsize i = 0; i < batch->count; ++i)
					Mem_CopyX16(&new_vertices[i * 8], &base_vertices[0]);
				
				for (intsize i = 0; i < batch->count; ++i)
				{
					new_indices[i*6 + 0] = (uint16)(i*4 + 0);
					new_indices[i*6 + 1] = (uint16)(i*4 + 1);
					new_indices[i*6 + 2] = (uint16)(i*4 + 2);
					new_indices[i*6 + 3] = (uint16)(i*4 + 2);
					new_indices[i*6 + 4] = (uint16)(i*4 + 1);
					new_indices[i*6 + 5] = (uint16)(i*4 + 3);
				}
				
				RB_UpdateVertexBuffer(rb, g_render_quadvbuf, BufMake(new_vertices_size, new_vertices));
				RB_UpdateIndexBuffer(rb, g_render_quadibuf, BufMake(new_indices_size, new_indices));
			}
		}
	}
	
	RB_CmdApplyPipeline(rb, g_render_quadpipeline);
	
	uint32 index_count;
	uint32 instance_count;
	
	if (g_render_caps.has_instancing)
	{
		index_count = 6;
		instance_count = batch->count;
	}
	else
	{
		index_count = batch->count * 6;
		instance_count = 0;
	}
	
	RB_CmdDraw(global_engine.renderbackend, &(RB_DrawDesc) {
		.ibuffer = g_render_quadibuf,
		.ubuffer = g_render_quadubuf,
		.vbuffers = { g_render_quadvbuf, g_render_quadelemsbuf, },
		.strides = { sizeof(int16[2]), sizeof(E_RectBatchElem), },
		.index_count = index_count,
		.instance_count = instance_count,
		.textures = {
			[0] = !RB_IsNull(batch->textures[0].handle) ? batch->textures[0].handle : g_render_whitetex,
			[1] = !RB_IsNull(batch->textures[1].handle) ? batch->textures[1].handle : g_render_whitetex,
			[2] = !RB_IsNull(batch->textures[2].handle) ? batch->textures[2].handle : g_render_whitetex,
			[3] = !RB_IsNull(batch->textures[3].handle) ? batch->textures[3].handle : g_render_whitetex,
		},
	});
}

API bool
E_PushText(E_RectBatch* batch, E_Font* font, String text, vec2 pos, vec2 scale, vec4 color)
{
	Trace();
	
	RB_Ctx* rb = global_engine.renderbackend;
	Arena* arena = batch->arena;
	SafeAssert(batch->elements + batch->count == (E_RectBatchElem*)Arena_End(arena));
	
	int32 int_texindex = -1;
	
	for (int32 i = 0; i < ArrayLength(batch->textures); ++i)
	{
		if (!RB_IsValid(rb, batch->textures[i].handle))
			int_texindex = i;
		
		if (RB_IsSame(rb, batch->textures[i].handle, font->texture.handle))
		{
			int_texindex = i;
			break;
		}
	}
	
	if (int_texindex == -1)
		return false;
	
	batch->textures[int_texindex] = font->texture;
	
	const float32 begin_x = pos[0];
	const float32 begin_y = pos[1];
	const float32 scale_x = font->char_scale * scale[0];
	const float32 scale_y = font->char_scale * scale[1];
	const float32 inv_bitmap_size = 1.0f / (float32)font->tex_size;
	
	float32 curr_x = begin_x;
	float32 curr_y = begin_y;
	
	uint32 codepoint;
	int32 str_index = 0;
	while (codepoint = String_Decode(text, &str_index), codepoint)
	{
		if (codepoint == ' ')
		{
			curr_x += (float32)font->space_advance * scale_x;
			continue;
		}
		
		if (codepoint == '\n')
		{
			curr_x = begin_x;
			curr_y += (float32)(font->ascent - font->descent + font->line_gap) * scale_y;
			continue;
		}
		
		if (codepoint <= 32)
			continue;
		
		// Hashmap find
		uint64 hash = Hash_IntHash64(codepoint);
		int32 index = (int32)hash;
		E_FontGlyphEntry* glyph = NULL;
		
		for (;;)
		{
			index = Hash_Msi(font->glyphmap_log2cap, hash, index);
			glyph = &font->glyphmap[index];
			
			if (!glyph->codepoint)
			{
				glyph = &font->invalid_glyph;
				break;
			}
			
			if (glyph->codepoint == codepoint)
				break;
		}
		
		SafeAssert(glyph);
		
		float32 x = curr_x + (float32)(glyph->xoff + glyph->bearing * font->char_scale) * scale[0];
		float32 y = curr_y + (float32)(glyph->yoff + font->ascent * font->char_scale) * scale[1];
		
		++batch->count;
		Arena_PushStructInit(arena, E_RectBatchElem, {
			.pos = { x, y, },
			.scaling = {
				[0][0] = (float32)glyph->width * scale[0],
				[1][1] = (float32)glyph->height * scale[1],
			},
			.tex_index = (int16)int_texindex,
			.tex_kind = 4,
			.texcoords = {
				(int16)((float32)glyph->x * inv_bitmap_size * INT16_MAX),
				(int16)((float32)glyph->y * inv_bitmap_size * INT16_MAX),
				(int16)((float32)glyph->width * inv_bitmap_size * INT16_MAX),
				(int16)((float32)glyph->height * inv_bitmap_size * INT16_MAX),
			},
			.color = { color[0], color[1], color[2], color[3], },
		});
		
		curr_x += (float32)glyph->advance * scale_x;
	}
	
	return true;
}

API void
E_PushRect(E_RectBatch* batch, const E_RectBatchElem* rect)
{
	Trace();
	Arena* arena = batch->arena;
	
	SafeAssert(batch->elements + batch->count == (E_RectBatchElem*)Arena_End(arena));
	
	++batch->count;
	Arena_PushStructData(arena, E_RectBatchElem, rect);
}

API bool
E_DecodeImage(Arena* output_arena, Buffer image, void** out_pixels, int32* out_width, int32* out_height)
{
	Trace();
	SafeAssert(image.size <= INT32_MAX);
	
	int32 width, height;
	void* temp_data;
	{
		Trace(); TraceName(Str("stbi_load_from_memory"));
		temp_data = stbi_load_from_memory(image.data, (int32)image.size, &width, &height, &(int32){ 0 }, 4);
	}
	if (!temp_data)
		return false;
	
	uintsize total_size = width*height*4;
	void* pixels = Arena_PushDirtyAligned(output_arena, total_size, 16);
	Mem_Copy(pixels, temp_data, total_size);
	
	stbi_image_free(temp_data);
	
	*out_pixels = pixels;
	*out_width = width;
	*out_height = height;
	
	return true;
}

API void
E_CalcTextSize(E_Font* font, String text, vec2 scale, vec2* out_size)
{
	Trace();
	vec2 size = { 0 };
	
	float32 scale_x = font->char_scale * scale[0];
	float32 scale_y = font->char_scale * scale[1];
	
	float32 curr_x = 0.0f;
	float32 curr_y = 0.0f;
	
	float32 max_x = 0.0f;
	
	int32 index = 0;
	uint32 codepoint;
	while (codepoint = String_Decode(text, &index), codepoint)
	{
		if (codepoint == ' ')
		{
			curr_x += (float32)font->space_advance * scale_x;
			continue;
		}
		
		if (codepoint == '\t')
		{
			curr_x += (float32)font->space_advance * scale_x * 4.0f;
			continue;
		}
		
		if (codepoint == '\n')
		{
			max_x = glm_max(max_x, curr_x);
			curr_x = 0.0f;
			curr_y += (float32)(font->ascent - font->descent + font->line_gap) * scale_y;
			continue;
		}
		
		if (codepoint <= 32)
			continue;
		
		// Hashmap find
		uint64 hash = Hash_IntHash64(codepoint);
		int32 index = (int32)hash;
		E_FontGlyphEntry* glyph = NULL;
		
		for (;;)
		{
			index = Hash_Msi(font->glyphmap_log2cap, hash, index);
			glyph = &font->glyphmap[index];
			
			if (!glyph->codepoint)
			{
				glyph = &font->invalid_glyph;
				break;
			}
			
			if (glyph->codepoint == codepoint)
				break;
		}
		
		curr_x += (float32)glyph->advance * scale_x;
	}
	
	size[0] = glm_max(max_x, curr_x);
	size[1] = curr_y + (float32)(font->ascent - font->descent + font->line_gap) * scale_y;
	
	glm_vec2_copy(size, *out_size);
}
