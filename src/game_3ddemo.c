internal void
ColorToVec4(uint32 color, vec4 out)
{
	out[0] = (float32)(color>>16 & 0xFF) / 255.0f;
	out[1] = (float32)(color>>8 & 0xFF) / 255.0f;
	out[2] = (float32)(color & 0xFF) / 255.0f;
	out[3] = (float32)(color>>24 & 0xFF) / 255.0f;
}

internal void
DrawGamepadLayout(const Input_Gamepad* gamepad, float32 x, float32 y, float32 width, float32 height)
{
	vec3 alignment = { -0.5f, -0.5f };
	vec4 black = { 0.1f, 0.1f, 0.1f, 1.0f };
	vec4 colors[2] = {
		{ 0.5f, 0.5f, 0.5f, 1.0f }, // Disabled
		{ 0.9f, 0.9f, 0.9f, 1.0f }, // Enabled
	};
	
	float32 xscale = width / 600.0f;
	float32 yscale = height / 600.0f;
	
	// NOTE(ljre): Draw Axes
	{
		Render_DrawRectangle(black,
							 (vec3) { x + width / 2.0f - 100.0f * xscale, y + height / 2.0f + 100.0f * yscale },
							 (vec3) { 50.0f * xscale, 50.0f * yscale }, alignment);
		
		Render_DrawRectangle(colors[Input_IsDown(*gamepad, Input_GamepadButton_LS)],
							 (vec3) { x + width / 2.0f - 100.0f * xscale + gamepad->left[0] * 20.0f * xscale,
								 y + height / 2.0f + 100.0f * yscale + gamepad->left[1] * 20.0f * yscale },
							 (vec3) { 20.0f * xscale, 20.0f * yscale }, alignment);
		
		Render_DrawRectangle(black,
							 (vec3) { x + width / 2.0f + 100.0f * xscale, y + height / 2.0f + 100.0f * yscale },
							 (vec3) { 50.0f * xscale, 50.0f * yscale }, alignment);
		
		Render_DrawRectangle(colors[Input_IsDown(*gamepad, Input_GamepadButton_RS)],
							 (vec3) { x + width / 2.0f + 100.0f * xscale + gamepad->right[0] * 20.0f * xscale, y + height / 2.0f + 100.0f * yscale + gamepad->right[1] * 20.0f * yscale },
							 (vec3) { 20.0f * xscale, 20.0f * yscale }, alignment);
	}
	
	// NOTE(ljre): Draw Buttons
	{
		struct Pair
		{
			vec3 pos;
			vec3 size;
		} buttons[] = {
			[Input_GamepadButton_Y] = { { width * 0.84f, height * 0.44f }, { 30.0f, 30.0f } },
			[Input_GamepadButton_B] = { { width * 0.90f, height * 0.50f }, { 30.0f, 30.0f } },
			[Input_GamepadButton_A] = { { width * 0.84f, height * 0.56f }, { 30.0f, 30.0f } },
			[Input_GamepadButton_X] = { { width * 0.78f, height * 0.50f }, { 30.0f, 30.0f } },
			
			[Input_GamepadButton_LB] = { { width * 0.20f, height * 0.30f }, { 60.0f, 20.0f } },
			[Input_GamepadButton_RB] = { { width * 0.80f, height * 0.30f }, { 60.0f, 20.0f } },
			
			[Input_GamepadButton_Up]= { { width * 0.16f, height * 0.44f }, { 30.0f, 30.0f } },
			[Input_GamepadButton_Right] = { { width * 0.22f, height * 0.50f }, { 30.0f, 30.0f } },
			[Input_GamepadButton_Down]  = { { width * 0.16f, height * 0.56f }, { 30.0f, 30.0f } },
			[Input_GamepadButton_Left]  = { { width * 0.10f, height * 0.50f }, { 30.0f, 30.0f } },
			
			[Input_GamepadButton_Back]  = { { width * 0.45f, height * 0.48f }, { 25.0f, 15.0f } },
			[Input_GamepadButton_Start] = { { width * 0.55f, height * 0.48f }, { 25.0f, 15.0f } },
		};
		
		for (int32 i = 0; i < ArrayLength(buttons); ++i)
		{
			buttons[i].pos[0] += x;
			buttons[i].pos[1] += y;
			buttons[i].size[0] *= xscale;
			buttons[i].size[1] *= yscale;
			
			Render_DrawRectangle(colors[Input_IsDown(*gamepad, i)], buttons[i].pos, buttons[i].size, alignment);
		}
	}
	
	// NOTE(ljre): Draw Triggers
	{
		vec4 color;
		
		glm_vec4_lerp(colors[0], colors[1], gamepad->lt, color);
		Render_DrawRectangle(color,
							 (vec3) { x + width * 0.20f, y + height * 0.23f },
							 (vec3) { 60.0f * xscale, 30.0f * yscale }, alignment);
		
		glm_vec4_lerp(colors[0], colors[1], gamepad->rt, color);
		Render_DrawRectangle(color,
							 (vec3) { x + width * 0.80f, y + height * 0.23f },
							 (vec3) { 60.0f * xscale, 30.0f * yscale }, alignment);
	}
}

#if 1
internal void
Game_3DDemoScene(Engine_Data* g)
{
	Trace("Game_3DDemoScene");
	
	Game_GlobalData* global = g->user_data;
	
	Asset_3DModel model;
	if (!Render_Load3DModelFromFile(Str("./assets/cube.glb"), &model))
	{
		Platform_ExitWithErrorMessage(Str("Could not load cube located in 'assets/cube.sobj'. Are you running this program on the build directory or repository directory? Make sure it's on the repository one."));
	}
	
	Asset_3DModel tree_model;
	if (!Render_Load3DModelFromFile(Str("./assets/first_tree.glb"), &tree_model))
	{
		Platform_ExitWithErrorMessage(Str("Could not load tree model :("));
	}
	
	Asset_3DModel corset_model;
	if (!Render_Load3DModelFromFile(Str("./assets/corset.glb"), &corset_model))
	{
		Platform_ExitWithErrorMessage(Str("Could not load corset model :("));
	}
	
	// NOTE(ljre): Load Audio
	Asset_SoundBuffer sound_music = { 0 };
	Asset_SoundBuffer sound_music3 = { 0 };
	Engine_LoadSoundBuffer(Str("music.ogg"), &sound_music);
	Engine_LoadSoundBuffer(Str("music3.ogg"), &sound_music3);
	
	Engine_PlayingAudio playing_audios[16];
	int32 playing_audio_count = 0;
	
	if (sound_music.samples)
	{
		playing_audios[0] = (Engine_PlayingAudio) {
			.sound = &sound_music,
			.frame_index = -1,
			.loop = true,
			.volume = 1.0f,
			.speed = 1.0f,
		};
		
		playing_audio_count = 1;
	}
	
	int32 controller_index = 0;
	
	float32 camera_yaw = 0.0f;
	float32 camera_pitch = 0.0f;
	float32 sensitivity = 0.05f;
	float32 camera_total_speed = 0.075f;
	float32 camera_height = 1.9f;
	float32 moving_time = 0.0f;
	vec2 camera_speed = { 0 };
	
	Render_Camera camera = {
		.pos = { 0.0f, camera_height, 0.0f },
		.dir = { 0.0f, 0.0f, -1.0f },
		.up = { 0.0f, 1.0f, 0.0f },
	};
	
	//~ NOTE(ljre): 3D Scene
	Render_3DModel* scene_models = Arena_Push(g->persistent_arena, sizeof(*scene_models) * 42);
	
	//- Ground
	Render_3DModel* ground = &scene_models[0];
	
	glm_mat4_identity(ground->transform);
	glm_scale(ground->transform, (vec3) { 20.0f, 2.0f, 20.0f });
	glm_translate(ground->transform, (vec3) { 0.0, -1.0f, 0.0f });
	ground->color[0] = 1.0f;
	ground->color[1] = 1.0f;
	ground->color[2] = 1.0f;
	ground->color[3] = 1.0f;
	ground->asset = &model;
	
	//- Tree
	Render_3DModel* tree = &scene_models[1];
	
	glm_mat4_identity(tree->transform);
	glm_translate(tree->transform, (vec3) { -1.0f });
	tree->asset = &tree_model;
	ColorToVec4(0xFFFFFFFF, tree->color);
	
	//- Corset
	for (int32 i = 0; i < 10; ++i) {
		Render_3DModel* corset = &scene_models[2 + i];
		
		vec3 pos = {
			Engine_RandomF32Range(-5.0f, 5.0f),
			0.1f,
			Engine_RandomF32Range(-5.0f, 5.0f),
		};
		
		glm_mat4_identity(corset->transform);
		glm_translate(corset->transform, pos);
		
		corset->asset = &corset_model;
		ColorToVec4(0xFFFFFFFF, corset->color);
	}
	
	//- Rotating Cubes
	Render_3DModel* rotating_cubes = &scene_models[12];
	int32 rotating_cube_count = 30;
	for (int32 i = 0; i < rotating_cube_count; ++i)
	{
		Render_3DModel* cube = &rotating_cubes[i];
		
		vec3 pos = {
			Engine_RandomF32Range(-20.0f, 20.0f),
			Engine_RandomF32Range(3.0f, 10.0f),
			Engine_RandomF32Range(-20.0f, 20.0f),
		};
		
		glm_mat4_identity(cube->transform);
		glm_translate(cube->transform, pos);
		cube->asset = &model;
		
		static const uint32 colors[] = {
			0xFFFF0000, 0xFFFF3E00, 0xFFFF8100, 0xFFFFC100, 0xFFFFFF00, 0xFFC1FF00,
			0xFF81FF00, 0xFF3EFF00, 0xFF00FF00, 0xFF00FF3E, 0xFF00FF81, 0xFF00FFC1,
			0xFF00FFFF, 0xFF00C1FF, 0xFF0081FF, 0xFF003EFF, 0xFF0000FF, 0xFF3E00FF,
			0xFF8100FF, 0xFFC100FF, 0xFFFF00FF, 0xFFFF00C1, 0xFFFF0081, 0xFFFF003E,
		};
		
		ColorToVec4(colors[Engine_RandomU64() % ArrayLength(colors)], cube->color);
	}
	
	//- Lights
	Render_3DPointLight lights[2];
	
	lights[0].position[0] = 1.0f;
	lights[0].position[1] = 2.0f;
	lights[0].position[2] = 5.0f;
	lights[0].ambient[0] = 0.2f;
	lights[0].ambient[1] = 0.0f;
	lights[0].ambient[2] = 0.0f;
	lights[0].diffuse[0] = 0.8f;
	lights[0].diffuse[1] = 0.0f;
	lights[0].diffuse[2] = 0.0f;
	lights[0].specular[0] = 1.0f;
	lights[0].specular[1] = 0.0f;
	lights[0].specular[2] = 0.0f;
	lights[0].constant = 1.0f;
	lights[0].linear = 0.09f;
	lights[0].quadratic = 0.032f;
	
	lights[1].position[0] = 1.0f;
	lights[1].position[1] = 2.0f;
	lights[1].position[2] = -5.0f;
	lights[1].ambient[0] = 0.0f;
	lights[1].ambient[1] = 0.0f;
	lights[1].ambient[2] = 1.0f;
	lights[1].diffuse[0] = 0.0f;
	lights[1].diffuse[1] = 0.0f;
	lights[1].diffuse[2] = 1.0f;
	lights[1].specular[0] = 0.0f;
	lights[1].specular[1] = 0.0f;
	lights[1].specular[2] = 0.8f;
	lights[1].constant = 1.0f;
	lights[1].linear = 0.09f;
	lights[1].quadratic = 0.032f;
	
	// NOTE(ljre): Render Command
	Render_3DScene scene3d = { 0 };
	scene3d.dirlight[0] = 1.0f;
	scene3d.dirlight[1] = 2.0f; // TODO(ljre): discover why tf this needs to be positive
	scene3d.dirlight[2] = 0.5f;
	glm_vec3_normalize(scene3d.dirlight);
	
	scene3d.light_model = &model;
	scene3d.point_lights = lights;
	scene3d.models = scene_models;
	scene3d.point_light_count = 2;
	scene3d.model_count = 42;
	
	while (!Platform_WindowShouldClose())
	{
		Trace("Game Loop");
		void* memory_state = Arena_End(g->temp_arena);
		
		if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape))
			break;
		
		if (Input_KeyboardIsPressed(Input_KeyboardKey_Left))
			controller_index = (controller_index - 1) & 15;
		else if (Input_KeyboardIsPressed(Input_KeyboardKey_Right))
			controller_index = (controller_index + 1) & 15;
		
		if (sound_music3.samples &&
			playing_audio_count < ArrayLength(playing_audios) &&
			Input_KeyboardIsPressed(Input_KeyboardKey_Space))
		{
			playing_audios[playing_audio_count++] = (Engine_PlayingAudio) {
				.sound = &sound_music3,
				.frame_index = -1,
				.loop = false,
				.volume = 1.0f,
				.speed = 1.5f,
			};
		}
		
		Input_Gamepad gamepad;
		bool32 is_connected = Input_GetGamepad(controller_index, &gamepad);
		
		float32 dt = Engine_DeltaTime();
		
		if (is_connected)
		{
			//~ NOTE(ljre): Camera Direction
			if (gamepad.right[0] != 0.0f)
			{
				camera_yaw += gamepad.right[0] * sensitivity * dt;
				camera_yaw = fmodf(camera_yaw, PI32 * 2.0f);
			}
			
			if (gamepad.right[1] != 0.0f)
			{
				camera_pitch += -gamepad.right[1] * sensitivity * dt;
				camera_pitch = glm_clamp(camera_pitch, -PI32 * 0.49f, PI32 * 0.49f);
			}
			
			float32 pitched = cosf(camera_pitch);
			
			camera.dir[0] = cosf(camera_yaw) * pitched;
			camera.dir[1] = sinf(camera_pitch);
			camera.dir[2] = sinf(camera_yaw) * pitched;
			glm_vec3_normalize(camera.dir);
			
			vec3 right;
			glm_vec3_cross((vec3) { 0.0f, 1.0f, 0.0f }, camera.dir, right);
			
			glm_vec3_cross(camera.dir, right, camera.up);
			
			//~ Movement
			vec2 speed;
			
			speed[0] = -gamepad.left[1];
			speed[1] =  gamepad.left[0];
			
			glm_vec2_rotate(speed, camera_yaw, speed);
			glm_vec2_scale(speed, camera_total_speed, speed);
			glm_vec2_lerp(camera_speed, speed, 0.2f * dt, camera_speed);
			
			camera.pos[0] += camera_speed[0] * dt;
			camera.pos[2] += camera_speed[1] * dt;
			
			if (Input_IsDown(gamepad, Input_GamepadButton_A))
				camera_height += 0.05f * dt;
			else if (Input_IsDown(gamepad, Input_GamepadButton_B))
				camera_height -= 0.05f * dt;
			
			//~ Bump
			float32 d = glm_vec2_distance2(camera_speed, GLM_VEC2_ZERO);
			if (d > 0.0001f)
			{
				moving_time += 0.3f * dt;
			}
			
			camera.pos[1] = camera_height + sinf(moving_time) * 0.04f;
		}
		
		if (is_connected)
			Render_ClearBackground(0.1f, 0.15f, 0.3f, 1.0f);
		else
			Render_ClearBackground(0.0f, 0.0f, 0.0f, 1.0f);
		
		//~ NOTE(ljre): 3D World
		float32 t = (float32)Platform_GetTime();
		
		for (int32 i = 0; i < rotating_cube_count; ++i)
		{
			vec4 pos;
			glm_vec4_copy(rotating_cubes[i].transform[3], pos);
			//glm_vec4_copy((vec4) { [3] = 1.0f }, rotating_cubes[i].transform[3]);
			
			//glm_mat4_transpose(rotating_cubes[i].transform);
			//glm_mat4_mulv(rotating_cubes[i].transform, pos, pos);
			
			glm_mat4_identity(rotating_cubes[i].transform);
			glm_translate(rotating_cubes[i].transform, pos);
			glm_rotate(rotating_cubes[i].transform, t + (float32)i, (vec3) { 0.0f, 1.0f, 0.0f });
		}
		
		Render_Draw3DScene(&scene3d, &camera);
		
		//~ NOTE(ljre): 2D User Interface
		Render_Begin2D();
		
		//- NOTE(ljre): Draw Controller Index
		const vec4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
		
		char buf[128];
		snprintf(buf, sizeof buf, "Controller Index: %i", controller_index);
		Render_DrawText(&global->font, Str(buf), (vec3) { 10.0f, 10.0f }, 32.0f, white, (vec3) { 0 });
		
		Render_DrawText(&global->font, Str("Press X, A, or B!"), (vec3) { 30.0f, 500.0f }, 24.0f, white, (vec3) { 0 });
		
		if (is_connected)
		{
			DrawGamepadLayout(&gamepad, 0.0f, 0.0f, 300.0f, 300.0f);
		}
		
		//~ NOTE(ljre): Finish Frame
		Engine_PlayAudios(playing_audios, &playing_audio_count, 0.075f);
		Engine_FinishFrame();
		
		Arena_Pop(g->temp_arena, memory_state);
	}
	
	g->current_scene = NULL;
}

#else

internal void
Game_3DDemoScene(Engine_Data* g)
{
	Trace("Game_3DDemoScene");
	
	Game_GlobalData* global = g->user_data;
	
	Asset_3DModel cube;
	if (!Render_Load3DModelFromFile(Str("./assets/cube.glb"), &cube))
	{
		Platform_ExitWithErrorMessage(Str("não deu pra carregar o cubo"));
	}
	
	const float32 width = 1.5f;
	const float32 height = 2.5f;
	const float32 deep = 50.0f;
	const float32 thic = 0.1f;
	
	float32 camera_yaw = PI32 / 2.0f;
	float32 camera_pitch = 0.0f;
	float32 sensitivity = 0.01f;
	float32 camera_total_speed = 0.025f;
	float32 camera_height = 1.9f;
	float32 moving_time = 0.0f;
	vec2 camera_speed = { 0 };
	
	Render_Camera camera = {
		.pos = { 0.0f, camera_height, -deep - 1.0f },
		.dir = { 0.0f, 0.0f, -1.0f },
		.up = { 0.0f, 1.0f, 0.0f },
	};
	
	//~ NOTE(ljre): 3D Scene
	Render_3DModel* scene_models = Arena_Push(g->persistent_arena, sizeof(*scene_models) * 4);
	
	{
		Render_3DModel* top = &scene_models[0];
		Render_3DModel* left = &scene_models[1];
		Render_3DModel* right = &scene_models[2];
		Render_3DModel* bottom = &scene_models[3];
		
		glm_mat4_identity(top->transform);
		glm_translate(top->transform, (vec3) { 0.0f, height*2.0f, 0.0f });
		glm_scale(top->transform, (vec3) { width, thic, deep });
		
		glm_mat4_identity(left->transform);
		glm_translate(left->transform, (vec3) { -width, height, 0.0f });
		glm_scale(left->transform, (vec3) { thic, height, deep });
		
		glm_mat4_identity(right->transform);
		glm_translate(right->transform, (vec3) { width, height, 0.0f });
		glm_scale(right->transform, (vec3) { thic, height, deep });
		
		glm_mat4_identity(bottom->transform);
		glm_scale(bottom->transform, (vec3) { width, thic, deep });
		glm_translate(bottom->transform, (vec3) { 0.0f, 0.0f, 0.0f });
		
		const float32 c = 0.15f;
		glm_vec4_copy((vec4) { c, c, c, 1.0f }, top->color);
		glm_vec4_copy((vec4) { c, c, c, 1.0f }, left->color);
		glm_vec4_copy((vec4) { c, c, c, 1.0f }, right->color);
		glm_vec4_copy((vec4) { c, c, c, 1.0f }, bottom->color);
		
		top->asset = left->asset = right->asset = bottom->asset = &cube;
	}
	
	//- Lights
	Render_3DPointLight lights[5];
	
	for (int32 i = 0; i < ArrayLength(lights); ++i)
	{
		lights[i].position[0] = 0.0f;
		lights[i].position[1] = height*1.9f;
		lights[i].position[2] = -deep + deep*2.0f / (float32)ArrayLength(lights) * (float32)i;
		
		lights[i].ambient[0] = 0.1f;
		lights[i].ambient[1] = 0.1f;
		lights[i].ambient[2] = 0.1f;
		
		lights[i].diffuse[0] = 1.0f;
		lights[i].diffuse[1] = 1.0f;
		lights[i].diffuse[2] = 1.0f;
		
		lights[i].specular[0] = 0.2f;
		lights[i].specular[1] = 0.2f;
		lights[i].specular[2] = 0.2f;
		
		lights[i].constant = 1.0f;
		lights[i].linear = 0.09f;
		lights[i].quadratic = 0.032f;
	}
	
	// NOTE(ljre): Render Command
	Render_3DScene scene3d = { 0 };
	scene3d.dirlight[0] = 0.0f;
	scene3d.dirlight[1] = 2.0f; // TODO(ljre): discover why tf this needs to be positive
	scene3d.dirlight[2] = 0.0f;
	glm_vec3_normalize(scene3d.dirlight);
	
	scene3d.light_model = &cube;
	scene3d.point_lights = lights;
	scene3d.models = scene_models;
	scene3d.point_light_count = ArrayLength(lights);
	scene3d.model_count = 4;
	
	while (!Platform_WindowShouldClose())
	{
		Trace("Game Loop");
		void* memory_state = Arena_End(g->temp_arena);
		
		if (Input_KeyboardIsPressed(Input_KeyboardKey_Escape))
			break;
		
		Input_Gamepad gamepad;
		bool32 is_connected = Input_GetGamepad(0, &gamepad);
		
		float32 dt = Engine_DeltaTime();
		
		if (is_connected)
		{
			//~ NOTE(ljre): Camera Direction
			if (gamepad.right[0] != 0.0f)
			{
				camera_yaw += gamepad.right[0] * sensitivity * dt;
				camera_yaw = fmodf(camera_yaw, PI32 * 2.0f);
			}
			
			if (gamepad.right[1] != 0.0f)
			{
				camera_pitch += -gamepad.right[1] * sensitivity * dt;
				camera_pitch = glm_clamp(camera_pitch, -PI32 * 0.49f, PI32 * 0.49f);
			}
			
			float32 pitched = cosf(camera_pitch);
			
			camera.dir[0] = cosf(camera_yaw) * pitched;
			camera.dir[1] = sinf(camera_pitch);
			camera.dir[2] = sinf(camera_yaw) * pitched;
			glm_vec3_normalize(camera.dir);
			
			vec3 right;
			glm_vec3_cross((vec3) { 0.0f, 1.0f, 0.0f }, camera.dir, right);
			
			glm_vec3_cross(camera.dir, right, camera.up);
			
			//~ Movement
			vec2 speed;
			
			speed[0] = -gamepad.left[1];
			speed[1] =  gamepad.left[0];
			
			glm_vec2_rotate(speed, camera_yaw, speed);
			glm_vec2_scale(speed, camera_total_speed, speed);
			glm_vec2_lerp(camera_speed, speed, 0.15f * dt, camera_speed);
			
			camera.pos[0] += camera_speed[0] * dt;
			camera.pos[2] += camera_speed[1] * dt;
			
			if (Input_IsDown(gamepad, Input_GamepadButton_A))
				camera_height += 0.05f * dt;
			else if (Input_IsDown(gamepad, Input_GamepadButton_B))
				camera_height -= 0.05f * dt;
			
			//~ Bump
			float32 d = glm_vec2_distance2(camera_speed, GLM_VEC2_ZERO);
			if (d > 0.0001f)
			{
				moving_time += 0.1f * dt;
			}
			
			camera.pos[1] = camera_height + sinf(moving_time) * 0.15f;
		}
		
		Render_ClearBackground(0.0f, 0.0f, 0.0f, 1.0f);
		
		//~ NOTE(ljre): 3D World
		float32 t = (float32)Platform_GetTime();
		
		Render_Draw3DScene(&scene3d, &camera);
		
		Engine_FinishFrame();
		Arena_Pop(g->temp_arena, memory_state);
	}
	
	g->current_scene = NULL;
}

#endif
