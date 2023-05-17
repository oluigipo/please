struct E_DecodeImageAsyncData_
{
	alignas(64) void* pixels;
	int32 width, height;
	
	intsize asset_index;
	Arena* output_arena;
	OS_RWLock* output_arena_lock;
	OS_MappedFile mapped_handle;
	Buffer mapped_contents;
}
typedef E_DecodeImageAsyncData_;

static void
E_DecodeImageAsync_(E_ThreadCtx* ctx, void* user_data)
{
	for ArenaTempScope(ctx->scratch_arena)
	{
		E_DecodeImageAsyncData_* data = user_data;
		Buffer encoded = data->mapped_contents;
		int32 width, height;
		void* pixels;
		
		if (stbi_info_from_memory(encoded.data, (int32)encoded.size, &width, &height, &(int32){0}))
		{
			uintsize size = (uintsize)width*height*4;
			
			if (data->output_arena_lock)
			{
				OS_LockExclusive(data->output_arena_lock);
				pixels = ArenaPushMemory(data->output_arena, pixels, size);
				OS_UnlockExclusive(data->output_arena_lock);
			}
			else
				pixels = ArenaPushMemory(data->output_arena, pixels, size);
			
			void* temp_data = stbi_load_from_memory(encoded.data, (int32)encoded.size, &width, &height, &(int32){0}, 4);
			MemoryCopy(pixels, temp_data, size);
			stbi_image_free(temp_data);
			
			data->pixels = pixels;
			data->width = width;
			data->height = height;
		}
		
		OS_UnmapFile(data->mapped_handle);
	}
}

API void
E_LoadAssets(E_AssetGroup* asset_group, Arena* scratch_arena)
{
	Arena* arena = asset_group->arena;
	
	if (!asset_group->tex2ds)
		asset_group->tex2ds = ArenaPushArray(arena, E_Tex2d, asset_group->tex2d_count);
	if (!asset_group->sounds)
		asset_group->sounds = ArenaPushArray(arena, E_SoundHandle, asset_group->sound_count);
	
	uint64 current_time = OS_CurrentPosixTime();
	
	//- Textures loading
	E_DecodeImageAsyncData_* jobs = ArenaEndAligned(scratch_arena, alignof(E_DecodeImageAsyncData_));
	intsize job_count = 0;
	
	for (intsize i = 0; i < asset_group->tex2d_count; ++i)
	{
		String path = asset_group->tex2ds_info[i].filepath;
		OS_MappedFile mapped_handle;
		Buffer mapped_contents;
		
		if ((RB_IsNull(asset_group->tex2ds[i].handle) || OS_IsFileOlderThan(path, current_time)) && OS_MapFile(path, &mapped_handle, &mapped_contents))
		{
			++job_count;
			ArenaPushStructInit(scratch_arena, E_DecodeImageAsyncData_, {
				.asset_index = i,
				.output_arena = arena,
				.output_arena_lock = &global_engine.mt_lock,
				.mapped_handle = mapped_handle,
				.mapped_contents = mapped_contents,
			});
		}
	}
	
	if (job_count == 1)
	{
		jobs[0].output_arena_lock = NULL;
		E_DecodeImageAsync_(&(E_ThreadCtx) { global_engine.scratch_arena }, &jobs[0]);
	}
	else if (job_count > 1)
	{
		for (intsize i = 0; i < job_count; ++i)
		{
			E_QueueThreadWork(&(E_ThreadWork) {
				.callback = E_DecodeImageAsync_,
				.data = &jobs[i],
			});
		}
		
		E_WaitRemainingThreadWork();
	}
	
	for (intsize i = 0; i < job_count; ++i)
	{
		intsize texindex = jobs[i].asset_index;
		if (!RB_IsNull(asset_group->tex2ds[texindex].handle))
			RB_FreeTexture2D(global_engine.renderbackend, asset_group->tex2ds[texindex].handle);
		
		asset_group->tex2ds[texindex] = (E_Tex2d) {
			.width = jobs[i].width,
			.height = jobs[i].height,
			.handle = RB_MakeTexture2D(global_engine.renderbackend, &(RB_Tex2dDesc) {
				.pixels = jobs[i].pixels,
				.width = jobs[i].width,
				.height = jobs[i].height,
				.format = RB_TexFormat_RGBA8,
				.flag_linear_filtering = true,
			}),
		};
	}
	
	ArenaPop(scratch_arena, jobs);
	asset_group->load_time = current_time;
}

API void
E_UnloadAssets(E_AssetGroup* asset_group)
{
	
}

