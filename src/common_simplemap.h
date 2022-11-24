#ifndef COMMON_SIMPLEMAP_H
#define COMMON_SIMPLEMAP_H

// NOTE(ljre): This is a super simple map implementation inspired by "MSI hash table".
//             https://nullprogram.com/blog/2022/08/08/

#ifndef SimpleMap_HashFunc
#   define SimpleMap_HashFunc SimpleMap_DefaultHashFunc_

static inline uint64
SimpleMap_DefaultHashFunc_(String memory)
{
	uint64 result = 14695981039346656037u;
	
	for (uintsize i = 0; i < memory.size; ++i)
	{
		uint64 value = (uint64)memory.data[i];
		
		result ^= value;
		result *= 1099511628211u;
	}
	
	return result;
}
#endif //SimpleMap_HashFunc

struct SimpleMap_Entry
{
	uint64 hash;
	void* data;
}
typedef SimpleMap_Entry;

struct SimpleMap
{
	uint32 len, log2_of_cap;
	SimpleMap_Entry entries[];
}
typedef SimpleMap;

static SimpleMap* SimpleMap_CreateFromArena(Arena* arena, uint32 log2_of_cap);
static SimpleMap* SimpleMap_CreateFromMemory(void* memory, uint32 log2_of_cap);
static SimpleMap* SimpleMap_RehashToArena(Arena* arena, uint32 log2_of_cap, SimpleMap* other);
static SimpleMap* SimpleMap_RehashToMemory(void* memory, uint32 log2_of_cap, SimpleMap* other);
static SimpleMap_Entry* SimpleMap_Find(SimpleMap* map, uint64 hash);
static SimpleMap_Entry* SimpleMap_Insert(SimpleMap* map, uint64 hash, void* data);
static int32 SimpleMap_Iterate(SimpleMap* map, uint64 hash, int32 index);

static SimpleMap*
SimpleMap_CreateFromArena(Arena* arena, uint32 log2_of_cap)
{
	Assert(log2_of_cap < 32);
	
	uint32 cap = 1 << log2_of_cap;
	SimpleMap* map = (SimpleMap*)Arena_PushAligned(arena, sizeof(SimpleMap) + sizeof(SimpleMap_Entry) * cap, alignof(SimpleMap));
	
	map->len = 0;
	map->log2_of_cap = log2_of_cap;
	
	return map;
}

static SimpleMap*
SimpleMap_CreateFromMemory(void* memory, uint32 log2_of_cap)
{
	Assert(log2_of_cap < 32);
	
	SimpleMap* map = (SimpleMap*)memory;
	
	map->len = 0;
	map->log2_of_cap = log2_of_cap;
	
	return map;
}

static SimpleMap*
SimpleMap_RehashToArena(Arena* arena, uint32 log2_of_cap, SimpleMap* other)
{
	Assert(other->log2_of_cap < log2_of_cap);
	SimpleMap* result = SimpleMap_CreateFromArena(arena, log2_of_cap);
	
	for (uint32 i = 0; i < (1 << other->log2_of_cap); ++i)
	{
		if (!other->entries[i].hash)
			continue;
		
		SimpleMap_Insert(result, other->entries[i].hash, other->entries[i].data);
		
		if (result->len == other->len)
			break;
	}
	
	return result;
}

static SimpleMap*
SimpleMap_RehashToMemory(void* memory, uint32 log2_of_cap, SimpleMap* other)
{
	Assert(other->log2_of_cap < log2_of_cap);
	SimpleMap* result = SimpleMap_CreateFromMemory(memory, log2_of_cap);
	
	for (uint32 i = 0; i < (1 << other->log2_of_cap); ++i)
	{
		if (!other->entries[i].hash)
			continue;
		
		SimpleMap_Insert(result, other->entries[i].hash, other->entries[i].data);
		
		if (result->len == other->len)
			break;
	}
	
	return result;
}

static int32
SimpleMap_Iterate(SimpleMap* map, uint64 hash, int32 index)
{
	uint32 exp = map->log2_of_cap;
	uint32 mask = (1u << exp) - 1;
	uint32 step = (uint32)(hash >> (64 - exp)) | 1;
	return (index + step) & mask;
}

static SimpleMap_Entry*
SimpleMap_Find(SimpleMap* map, uint64 hash)
{
	Assert(hash != 0);
	uint32 security_counter = 0;
	
	for (int32 i = (int32)hash;;)
	{
		i = SimpleMap_Iterate(map, hash, i);
		
		if (map->entries[i].hash == hash)
			return &map->entries[i];
		if (!map->entries[i].hash)
			return NULL;
		if (++security_counter > map->len)
			return NULL;
	}
}

static SimpleMap_Entry*
SimpleMap_Insert(SimpleMap* map, uint64 hash, void* data)
{
	Assert(hash != 0);
	uint32 cap = (1 << map->log2_of_cap);
	uint32 security_counter = 0;
	SimpleMap_Entry* entry = NULL;
	
	if (map->len >= cap)
		return NULL;
	
	for (int32 i = (int32)hash;;)
	{
		i = SimpleMap_Iterate(map, hash, i);
		
		if (map->entries[i].hash == hash || !map->entries[i].hash)
		{
			entry = &map->entries[i];
			break;
		}
		
		if (++security_counter > cap)
			return NULL;
	}
	
	entry->hash = hash;
	entry->data = data;
	
	return entry;
}

#endif //COMMON_SIMPLEMAP_H
