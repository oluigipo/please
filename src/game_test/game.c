#include "api_engine.h"

static G_GlobalData* game;
static E_GlobalData* engine;

struct G_GlobalData
{
	Render_Texture2D texture;
};

static void
G_Init(void)
{
	game = engine->game = Arena_PushStruct(engine->persistent_arena, G_GlobalData);
	
	
}

static void
G_UpdateAndRender(void)
{
	if (engine->window_state->should_close || OS_IsPressed(engine->input->keyboard, OS_KeyboardKey_Escape))
		engine->running = false;
	
	Render_ClearColor(engine, (vec4) { 0.2f, 0.0f, 0.4f, 1.0f });
	
	Render_Data2DInstance* instances;
	uintsize instance_count;
	{
		instances = Arena_EndAligned(engine->scratch_arena, alignof(Render_Data2DInstance));
		
		for (int32 i = 0; i < 100; ++i)
		{
			Render_Data2DInstance inst = {
				.transform = GLM_MAT4_IDENTITY_INIT,
				.texcoords = { 0.0f, 0.0f, 1.0f, 1.0f },
				.color = { 1.0f, 1.0f, 1.0f, 1.0f }
			};
			
			uint64 random = Hash_IntHash64(i);
			
			float32 x = (random & 511) - 256.0f;
			float32 y = (random>>9 & 511) - 256.0f;
			float32 angle = (random>>18 & 511) / 512.0f * (float32)M_PI*2;
			vec3 color = {
				(random>>27 & 255) / 255.0f,
				(random>>35 & 255) / 255.0f,
				(random>>43 & 255) / 255.0f,
			};
			
			glm_translate(inst.transform, (vec3) { x, y });
			glm_rotate(inst.transform, angle, (vec3) { 0, 0, 1.0f });
			glm_scale(inst.transform, (vec3) { 32.0f, 32.0f, 1.0f });
			
			glm_vec4_copy(color, inst.color);
			
			Arena_PushData(engine->scratch_arena, &inst);
		}
		
		instance_count = (Render_Data2DInstance*)Arena_End(engine->scratch_arena) - instances;
	}
	
	Render_Data2D data2d = {
		.instance_count = instance_count,
		.instances = instances,
	};
	
	Render_Draw2D(engine, &data2d);
	
	E_FinishFrame();
}

API void
G_Main(E_GlobalData* data)
{
	engine = data;
	game = data->game;
	
	if (!game)
		G_Init();
	
	for Arena_TempScope(data->scratch_arena)
		G_UpdateAndRender();
}
