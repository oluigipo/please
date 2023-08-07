#ifndef COMMON_STORAGE_H
#define COMMON_STORAGE_H

struct Storage_Handle
{
	uint16 generation;
	uint16 index; // 1-index based
}
typedef Storage_Handle;

struct Storage_Block
{
	uint16 generation;
	uint16 next_free; // 1-index-based
	
	uint32 offset;
	uint32 size;
	uint32 free_size;
}
typedef Storage_Block;

struct Storage
{
	uint8* backbuffer;
	
	uint32 block_first_free;
	uint32 block_count;
	uint32 block_cap;
	
	uint32 memory_offset;
	uint32 memory_size;
}
typedef Storage;

#ifndef Storage_GRANULARITY
#	define Storage_GRANULARITY 128
#endif

static Storage Storage_MakeFromMemory(Buffer backbuffer, uint32 max_allocations);
static bool Storage_Alloc(Storage* storage, uintsize desired_size, Storage_Handle* out_handle, void** out_buffer, uintsize* out_size);
static bool Storage_Fetch(Storage* storage, Storage_Handle handle, void** out_buffer, uintsize* out_size);
static bool Storage_Dealloc(Storage* storage, Storage_Handle handle);
static void Storage_Defrag(Storage* storage, Arena* scratch_arena);

//~ Implementation
static Storage
Storage_MakeFromMemory(Buffer backbuffer, uint32 max_allocations)
{
	Trace();
	SafeAssert(max_allocations < UINT16_MAX);
	
	// The memory should start at an offset multiple of the minimum granularity.
	uint32 memory_offset = AlignUp(sizeof(Storage_Block)*max_allocations, Storage_GRANULARITY-1);
	max_allocations = memory_offset / sizeof(Storage_Block);
	SafeAssert(backbuffer.data && backbuffer.size >= memory_offset);
	
	// It's also important that the size is a multiple of minumum granularity.
	uintsize memory_size = backbuffer.size - memory_offset;
	SafeAssert(memory_size <= UINT32_MAX && (memory_size & Storage_GRANULARITY-1) == 0);
	
	Storage storage = {
		.backbuffer = (uint8*)backbuffer.data,
		.block_count = 1,
		.block_cap = max_allocations,
		
		.memory_offset = memory_offset,
		.memory_size = (uint32)memory_size,
	};
	
	// Base allocation.
	Storage_Block* blocks = (Storage_Block*)storage.backbuffer;
	MemoryZero(blocks, sizeof(Storage_Block) * storage.block_cap);
	
	blocks[0].generation = 0;
	blocks[0].next_free = 0;
	blocks[0].offset = 0;
	blocks[0].size = 0;
	blocks[0].free_size = (uint32)memory_size;
	
	return storage;
}

static bool
Storage_Alloc(Storage* storage, uintsize desired_size, Storage_Handle* out_handle, void** out_buffer, uintsize* out_size)
{
	Trace();
	SafeAssert(storage && storage->backbuffer);
	SafeAssert(out_handle && desired_size <= UINT32_MAX);
	
	bool result = false;
	Storage_Handle handle = { 0 };
	void* buffer = NULL;
	uintsize size = 0;
	
	Storage_Block* blocks = (Storage_Block*)storage->backbuffer;
	uint32 actual_desired_size = AlignUp((uint32)desired_size, Storage_GRANULARITY-1);
	
	for (intsize i = 0; i < storage->block_count; ++i)
	{
		Storage_Block* block = &blocks[i];
		if (block->next_free != 0)
			continue;
		
		if (block->free_size >= actual_desired_size)
		{
			uint32 alloc_index;
			
			if (storage->block_first_free)
			{
				alloc_index = storage->block_first_free-1;
				storage->block_first_free = blocks[alloc_index].next_free-1;
			}
			else if (storage->block_count < storage->block_cap)
				alloc_index = storage->block_count++;
			else
				// There's no more space for individual allocations...
				break;
			
			Storage_Block* alloc_block = &blocks[alloc_index];
			alloc_block->next_free = 0;
			alloc_block->offset = block->offset + block->size;
			alloc_block->size = actual_desired_size;
			alloc_block->free_size = block->free_size - actual_desired_size;
			
			block->free_size = 0;
			
			handle.generation = alloc_block->generation;
			handle.index = (uint16)(alloc_index + 1);
			
			result = true;
			buffer = storage->backbuffer + storage->memory_offset + alloc_block->offset;
			size = alloc_block->size;
			break;
		}
	}
	
	if (out_buffer)
		*out_buffer = buffer;
	if (out_size)
		*out_size = size;
	
	*out_handle = handle;
	return result;
}

static bool
Storage_Fetch(Storage* storage, Storage_Handle handle, void** out_buffer, uintsize* out_size)
{
	Trace();
	SafeAssert(storage && storage->backbuffer);
	
	if (handle.index == 0 || handle.index > storage->block_count)
		return false;
	
	Storage_Block* blocks = (Storage_Block*)storage->backbuffer;
	Storage_Block* wanted_block = &blocks[handle.index-1];
	
	if (wanted_block->next_free != 0 || wanted_block->generation != handle.generation)
		return false;
	
	void* buffer = storage->backbuffer + storage->memory_offset + wanted_block->offset;
	uintsize size = wanted_block->size;
	
	if (out_buffer)
		*out_buffer = buffer;
	if (out_size)
		*out_size = size;
	
	return true;
}

static bool
Storage_Dealloc(Storage* storage, Storage_Handle handle)
{
	Trace();
	SafeAssert(storage && storage->backbuffer);
	
	if (handle.index == 0 || handle.index > storage->block_count)
		return false;
	
	Storage_Block* blocks = (Storage_Block*)storage->backbuffer;
	Storage_Block* wanted_block = &blocks[handle.index-1];
	
	if (wanted_block->next_free != 0 || wanted_block->generation != handle.generation)
		return false;
	
	bool found = false;
	for (intsize i = 0; i < storage->block_count; ++i)
	{
		Storage_Block* block = &blocks[i];
		
		// Check if this block is right behind our wanted_block.
		if (block->offset + block->size + block->free_size == wanted_block->offset)
		{
			block->free_size += wanted_block->size + wanted_block->free_size;
			found = true;
			break;
		}
	}
	
	Assert(found); // NOTE(ljre): I still have to think this through, but 'found' should never be false.
	
	if (found)
	{
		wanted_block->next_free = (uint16)storage->block_first_free + 1;
		storage->block_first_free = handle.index;
		
		wanted_block->generation++;
		wanted_block->offset = 0;
		wanted_block->size = 0;
		wanted_block->free_size = 0;
	}
	
	return found;
}

static void
Storage_Defrag(Storage* storage, Arena* scratch_arena)
{
	Trace();
	SafeAssert(storage && storage->backbuffer);
	
	Storage_Block* storage_blocks = (Storage_Block*)storage->backbuffer;
	ArenaSavepoint scratch_save = ArenaSave(scratch_arena);
	
	// Set blocks
	Storage_Block** blocks_array = ArenaPushArray(scratch_arena, Storage_Block*, storage->block_count);
	int32 block_count = 0;
	
	for (intsize i = 0; i < storage->block_count; ++i)
	{
		if (storage_blocks[i].next_free == 0)
			blocks_array[block_count++] = &storage_blocks[i];
	}
	
	// Sort by offset
	for (intsize i = 1; i < block_count; ++i)
	{
		for (intsize j = 1; j < block_count-i; j++)
		{
			if (blocks_array[j]->offset > blocks_array[j+1]->offset)
			{
				Storage_Block* temp = blocks_array[j];
				blocks_array[j] = blocks_array[j+1];
				blocks_array[j+1] = temp;
			}
		}
	}
	
	// Compact them
	for (intsize i = 0; i < block_count-1; ++i)
	{
		if (blocks_array[i]->free_size > 0)
		{
			uint32 move_size = blocks_array[i+1]->size;
			uint32 target_offset = blocks_array[i]->offset + blocks_array[i]->size;
			uint32 source_offset = blocks_array[i+1]->offset;
			
			MemoryMove(storage->backbuffer + storage->memory_offset + target_offset, storage->backbuffer + storage->memory_offset + source_offset, move_size);
			
			blocks_array[i+1]->offset = target_offset;
			blocks_array[i+1]->free_size += blocks_array[i]->free_size;
			blocks_array[i]->free_size = 0;
		}
	}
	
	// Done
	ArenaRestore(scratch_save);
}

#endif //COMMON_STORAGE_H
