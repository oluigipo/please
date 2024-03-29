struct G_Scene3DState
{
	RB_Shader shader;
	RB_UBuffer shader_ubuffer;
	RB_Pipeline pipeline;
	
	RB_VBuffer tree_model_vbuffer;
	RB_IBuffer tree_model_ibuffer;
	RB_Tex2d tree_model_diffuse_texture;
	//RB_Handle tree_model_normal_texture;
	mat4 tree_model_base_transform;
	vec4 tree_model_base_color;
	int32 tree_model_index_count;
	RB_IndexType tree_model_index_type;
	
	uint32 tree_model_position_offset;
	uint32 tree_model_texcoord_offset;
	uint32 tree_model_normal_offset;
	
	float32 camera_yaw;
	float32 camera_pitch;
	vec3 camera_pos;
};

struct G_Scene3DUBuffer
{
	mat4 view;
	mat4 model;
}
typedef G_Scene3DUBuffer;

static const char g_scene3d_gl_vs[] =
"layout (location = 0) in vec3 aPosition;\n"
"layout (location = 1) in vec2 aTexcoord;\n"
"layout (location = 2) in vec3 aNormal;\n"
"\n"
"out vec2 vTexcoord;\n"
"out vec3 vNormal;\n"
"\n"
"uniform UniformBuffer {\n"
"    mat4 uView;\n"
"    mat4 uModel;\n"
"};\n"
"\n"
"void main() {\n"
"    gl_Position = uView * uModel * vec4(aPosition, 1.0);\n"
"    vTexcoord = aTexcoord;\n"
"    vNormal = aNormal;\n"
"    \n"
"}\n"
"\n";

static const char g_scene3d_gl_fs[] =
"in vec2 vTexcoord;\n"
"in vec3 vNormal;\n"
"\n"
"out vec4 oFragColor;\n"
"\n"
"uniform sampler2D uTexture[4];\n"
"\n"
"void main() {\n"
"    oFragColor = texture(uTexture[0], vTexcoord);\n"
"}\n"
"\n";

uint8 typedef BYTE;
#include <d3d11_gametest_scene3d_vs.inc>
#include <d3d11_gametest_scene3d_ps.inc>
#include <d3d11_gametest_scene3d_level91_vs.inc>
#include <d3d11_gametest_scene3d_level91_ps.inc>

static void
G_Scene3DInit(void)
{
	Trace();
	
	engine->os->window.show_cursor = false;
	engine->os->window.lock_cursor = true;
	
	G_Scene3DState* s = game->scene3d = ArenaPushStructInit(engine->persistent_arena, G_Scene3DState, {
		.camera_yaw = -GLM_PIf*0.5f,
		.camera_pitch = 0.0f,
		.camera_pos = { 0.0f, 0.0f, 5.0f, },
	});
	
	// NOTE(ljre): Setup test renderer
	s->shader = RB_MakeShader(engine->renderbackend, &(RB_ShaderDesc) {
		.hlsl40 = {
			BufInit(g_scene3d_d3d11_gametest_scene3d_vs),
			BufInit(g_scene3d_d3d11_gametest_scene3d_ps),
		},
		.hlsl40_91 = {
			BufInit(g_scene3d_d3d11_gametest_scene3d_level91_vs),
			BufInit(g_scene3d_d3d11_gametest_scene3d_level91_ps),
		},
		.glsl = {
			StrInit(g_scene3d_gl_vs),
			StrInit(g_scene3d_gl_fs),
		},
	});
	
	s->pipeline = RB_MakePipeline(engine->renderbackend, &(RB_PipelineDesc) {
		.cull_mode = RB_CullMode_Back,
		.flag_depth_test = true,
		
		.shader = s->shader,
		.input_layout = {
			[0] = {
				.format = RB_VertexFormat_Vec3,
				.buffer_slot = 0,
			},
			[1] = {
				.format = RB_VertexFormat_Vec2,
				.buffer_slot = 1,
			},
			[2] = {
				.format = RB_VertexFormat_Vec3,
				.buffer_slot = 2,
			},
		},
	});
	
	s->shader_ubuffer = RB_MakeUniformBuffer(engine->renderbackend, &(RB_UBufferDesc) {
		.size = sizeof(G_Scene3DUBuffer),
	});
	
	// NOTE(ljre): Load test model
	{
		Buffer gltf_data = { 0 };
		
#ifdef CONFIG_ENABLE_EMBED
		gltf_data = BufRange(g_corset_model_begin, g_corset_model_end);
#else
		SafeAssert(OS_ReadEntireFile(Str("assets/corset.glb"), engine->scratch_arena, (void**)&gltf_data.data, &gltf_data.size));
#endif
		
		UGltf_JsonRoot gltf = { 0 };
		SafeAssert(UGltf_Parse(gltf_data.data, gltf_data.size, engine->scratch_arena, &gltf));
		
		UGltf_JsonPrimitive* prim = &gltf.meshes[gltf.nodes[gltf.scenes[gltf.scene].nodes[0]].mesh].primitives[0];
		UGltf_JsonBufferView* view;
		intsize total_size = 0;
		
		//- Position
		view = &gltf.buffer_views[gltf.accessors[prim->attributes.position].buffer_view];
		s->tree_model_position_offset = (uint32)total_size;
		total_size += view->byte_length;
		Buffer position_buffer = {
			.size = view->byte_length,
			.data = gltf.buffers[view->buffer].data + view->byte_offset,
		};
		
		//- Texcoord
		view = &gltf.buffer_views[gltf.accessors[prim->attributes.texcoord_0].buffer_view];
		s->tree_model_texcoord_offset = (uint32)total_size;
		total_size += view->byte_length;
		Buffer texcoord_buffer = {
			.size = view->byte_length,
			.data = gltf.buffers[view->buffer].data + view->byte_offset,
		};
		
		//- Normal
		view = &gltf.buffer_views[gltf.accessors[prim->attributes.normal].buffer_view];
		s->tree_model_normal_offset = (uint32)total_size;
		total_size += view->byte_length;
		Buffer normal_buffer = {
			.size = view->byte_length,
			.data = gltf.buffers[view->buffer].data + view->byte_offset,
		};
		
		//- Concat
		uint8* memory = ArenaPushDirty(engine->frame_arena, total_size);
		MemoryCopy(memory + s->tree_model_position_offset, position_buffer.data, position_buffer.size);
		MemoryCopy(memory + s->tree_model_texcoord_offset, texcoord_buffer.data, texcoord_buffer.size);
		MemoryCopy(memory + s->tree_model_normal_offset,   normal_buffer.data,   normal_buffer.size  );
		
		s->tree_model_vbuffer = RB_MakeVertexBuffer(engine->renderbackend, &(RB_VBufferDesc) {
			.initial_data = memory,
			.size = total_size,
		});
		
		//- Misc
		SafeAssert(prim->mode == 4); // GL_TRIANGLES
		s->tree_model_index_count = gltf.accessors[prim->indices].count;
		glm_mat4_copy(gltf.nodes[gltf.scenes[gltf.scene].nodes[0]].transform, s->tree_model_base_transform);
		
		switch (gltf.accessors[prim->indices].component_type)
		{
			case 0x1403: s->tree_model_index_type = RB_IndexType_Uint16; break;
			case 0x1405: s->tree_model_index_type = RB_IndexType_Uint32; break;
			default: SafeAssert(false); break;
		}
		
		//- Index buffer
		view = &gltf.buffer_views[gltf.accessors[prim->indices].buffer_view];
		intsize ibuffer_size = view->byte_length;
		void* ibuffer_data = ArenaPushMemory(engine->frame_arena, gltf.buffers[view->buffer].data + view->byte_offset, ibuffer_size);
		
		s->tree_model_ibuffer = RB_MakeIndexBuffer(engine->renderbackend, &(RB_IBufferDesc) {
			.initial_data = ibuffer_data,
			.size = ibuffer_size,
			.index_type = s->tree_model_index_type,
		});
		
		//- Material
		SafeAssert(prim->material != -1);
		UGltf_JsonMaterial* mat = &gltf.materials[prim->material];
		glm_vec4_copy(mat->pbr_metallic_roughness.base_color_factor, s->tree_model_base_color);
		
		//- Material Base Color Texture
		SafeAssert(mat->pbr_metallic_roughness.base_color_texture.specified);
		UGltf_JsonTexture* tex = &gltf.textures[mat->pbr_metallic_roughness.base_color_texture.index];
		UGltf_JsonSampler* sampler = &gltf.samplers[tex->sampler];
		UGltf_JsonImage* image = &gltf.images[tex->source];
		UGltf_JsonBufferView* bufferview = &gltf.buffer_views[image->buffer_view];
		(void)sampler;
		
		Buffer encoded_image = {
			.size = bufferview->byte_length,
			.data = gltf.buffers[bufferview->buffer].data + bufferview->byte_offset,
		};
		
		int32 imgwidth, imgheight;
		void* imgdata;
		SafeAssert(E_DecodeImage(engine->frame_arena, encoded_image, &imgdata, &imgwidth, &imgheight));
		
		s->tree_model_diffuse_texture = RB_MakeTexture2D(engine->renderbackend, &(RB_Tex2dDesc) {
			.pixels = imgdata,
			.width = imgwidth,
			.height = imgheight,
			.format = RB_TexFormat_RGBA8,
			.flag_linear_filtering = true,
		});
	}
}

static void
G_Scene3DUpdateAndRender(void)
{
	G_Scene3DState* s = game->scene3d;
	
	s->camera_yaw   +=  0.002f * (engine->os->mouse.old_pos[0] - engine->os->mouse.pos[0]);
	s->camera_pitch += -0.002f * (engine->os->mouse.old_pos[1] - engine->os->mouse.pos[1]);
	
	s->camera_yaw   = fmodf(s->camera_yaw, GLM_PIf*2.0f);
	s->camera_pitch = glm_clamp(s->camera_pitch, -GLM_PIf*0.49f, GLM_PIf*0.49f);
	
	float32 pitched = cosf(s->camera_pitch);
	vec3 dir = {
		cosf(s->camera_yaw) * pitched,
		sinf(s->camera_pitch),
		sinf(s->camera_yaw) * pitched,
	};
	
	vec3 up = { 0.0f, 1.0f, 0.0f };
	vec3 right;
	glm_vec3_cross(up, dir, right);
	glm_vec3_cross(dir, right, up);
	
	vec3 move = {
		OS_IsDown(engine->os->keyboard, 'W') - OS_IsDown(engine->os->keyboard, 'S'),
		OS_IsDown(engine->os->keyboard, OS_KeyboardKey_LeftShift) - OS_IsDown(engine->os->keyboard, ' '),
		OS_IsDown(engine->os->keyboard, 'A') - OS_IsDown(engine->os->keyboard, 'D'),
	};
	
	glm_vec3_rotate(move, -s->camera_yaw, vec3(0.0f, 1.0f, 0.0f));
	glm_vec3_scale(move, engine->delta_time*5.0f, move);
	glm_vec3_add(s->camera_pos, move, s->camera_pos);
	
	// NOTE(ljre): Run test renderer
	{
		G_Scene3DUBuffer* ubuffer_data = ArenaPushStruct(engine->frame_arena, G_Scene3DUBuffer);
		
		E_Camera3D camera = { 0 };
		glm_vec3_copy(s->camera_pos, camera.pos);
		glm_vec3_copy(up, camera.up);
		glm_vec3_copy(dir, camera.dir);
		
		E_CalcViewMatrix3D(&camera, ubuffer_data->view, 80.0f, 16.0f/9.0f);
		E_CalcModelMatrix3D(vec3(0), vec3(1.0f, -1.0f, 1.0f), vec3(0), ubuffer_data->model);
		glm_mat4_mul(s->tree_model_base_transform, ubuffer_data->model, ubuffer_data->model);
		
		RB_UpdateUniformBuffer(engine->renderbackend, s->shader_ubuffer, BufMake(sizeof(*ubuffer_data), ubuffer_data));
		RB_CmdApplyPipeline(engine->renderbackend, s->pipeline);
		RB_CmdDraw(engine->renderbackend, &(RB_DrawDesc) {
			.ibuffer = s->tree_model_ibuffer,
			.ubuffer = s->shader_ubuffer,
			.vbuffers = { s->tree_model_vbuffer, s->tree_model_vbuffer, s->tree_model_vbuffer },
			.offsets = {
				s->tree_model_position_offset,
				s->tree_model_texcoord_offset,
				s->tree_model_normal_offset,
			},
			.strides = {
				sizeof(float32)*3,
				sizeof(float32)*2,
				sizeof(float32)*3,
			},
			
			.index_count = s->tree_model_index_count,
			
			.textures = {
				s->tree_model_diffuse_texture,
				//s->tree_model_normal_texture,
			},
		});
	}
}
