//~ Internal API
static bool
Engine_InitRender(const Engine_RenderApi** out_api)
{
	Trace();
	
	switch (global_engine.graphics_context->api)
	{
		case Engine_GraphicsApi_None: return false;
#ifdef INTERNAL_ENABLE_OPENGL
		case Engine_GraphicsApi_OpenGL: return OpenGL_Init(out_api);
#endif
#ifdef INTERNAL_ENABLE_D3D11
		case Engine_GraphicsApi_Direct3D: return D3d11_Init(out_api);
#endif
		default: Unreachable(); break;
	}
	
	return false;
}

static void
Engine_DeinitRender(void)
{
	Trace();
	
	switch (global_engine.graphics_context->api)
	{
		case Engine_GraphicsApi_None: break;
#ifdef INTERNAL_ENABLE_OPENGL
		case Engine_GraphicsApi_OpenGL: OpenGL_Deinit(); break;
#endif
#ifdef INTERNAL_ENABLE_D3D11
		case Engine_GraphicsApi_Direct3D: D3d11_Deinit(); break;
#endif
		default: Unreachable(); break;
	}
}

//~ API
API void
Engine_CalcViewMatrix2D(const Render_Camera2D* camera, mat4 out_view)
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
Engine_CalcViewMatrix3D(const Render_Camera3D* camera, mat4 out_view, float32 fov, float32 aspect)
{
	glm_mat4_identity(out_view);
	glm_look((float32*)camera->pos, (float32*)camera->dir, (float32*)camera->up, out_view);
	
	mat4 proj;
	glm_perspective(fov, aspect, 0.01f, 100.0f, proj);
	glm_mat4_mul(proj, out_view, out_view);
}

API void
Engine_CalcModelMatrix2D(const vec2 pos, const vec2 scale, float32 angle, mat4 out_model)
{
	mat4 model;
	glm_mat4_identity(model);
	glm_translate(model, (vec3) { pos[0], pos[1] });
	glm_scale(model, (vec3) { scale[0], scale[1], 1.0f });
	glm_rotate(model, angle, (vec3) { 0.0f, 0.0f, 1.0f });
	glm_mat4_copy(model, out_model);
}

API void
Engine_CalcModelMatrix3D(const vec3 pos, const vec3 scale, const vec3 rot, mat4 out_model)
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
Engine_CalcPointInCamera2DSpace(const Render_Camera2D* camera, const vec2 pos, vec2 out_pos)
{
	vec2 result;
	float32 inv_zoom = 1.0f / camera->zoom * 2.0f;
	
	result[0] = camera->pos[0] + (pos[0] - camera->size[0] * 0.5f) * inv_zoom;
	result[1] = camera->pos[1] + (pos[1] - camera->size[1] * 0.5f) * inv_zoom;
	//result[1] *= -1.0f;
	
	glm_vec2_copy(result, out_pos);
}
