struct GltfJson_Scene
{
	String name;
	int32* nodes;
	uintsize node_count;
}
typedef GltfJson_Scene;

struct GltfJson_Node
{
	String name;
	int32 mesh;
	
	mat4 transform;
}
typedef GltfJson_Node;

struct GltfJson_TextureInfo
{
	bool32 specified;
	
	int32 index;
	int32 texcoord; // default: 0
}
typedef GltfJson_TextureInfo;

struct GltfJson_MetallicRoughness
{
	vec4 base_color_factor; // default: { 1, 1, 1, 1 }
	
	float32 metalic_factor; // detault: 1
	float32 roughness_factor; // default: 1
	
	GltfJson_TextureInfo base_color_texture;
	GltfJson_TextureInfo metallic_roughness_texture;
}
typedef GltfJson_MetallicRoughness;

struct GltfJson_NormalTextureInfo
{
	bool32 specified;
	
	int32 index;
	int32 texcoord; // default: 0
	float32 scale; // default: 1
}
typedef GltfJson_NormalTextureInfo;

struct GltfJson_OcclusionTextureInfo
{
	bool32 specified;
	
	int32 index;
	int32 texcoord; // default: 0
	float32 strength; // default: 1
}
typedef GltfJson_OcclusionTextureInfo;

struct GltfJson_Material
{
	String name;
	GltfJson_MetallicRoughness pbr_metallic_roughness;
	GltfJson_NormalTextureInfo normal_texture;
	GltfJson_OcclusionTextureInfo occlusion_texture;
	GltfJson_TextureInfo emissive_texture;
	
	vec3 emissive_factor; // default: { 0, 0, 0 }
	String alpha_mode; // default: "OPAQUE"
	float32 alpha_cutoff; // default: 0.5
	bool32 double_sided; // default: false
}
typedef GltfJson_Material;

struct GltfJson_Primitive
{
	// NOTE(ljre): -1 for "undefined"
	int32 indices;
	int32 mode; // default: 4
	int32 material;
	
	struct {
		int32 position;
		int32 normal;
		int32 tangent;
		int32 texcoord_0;
	} attributes;
	
}
typedef GltfJson_Primitive;

struct GltfJson_Mesh
{
	String name;
	GltfJson_Primitive* primitives;
	uintsize primitive_count;
}
typedef GltfJson_Mesh;

struct GltfJson_Sampler
{
	String name;
	
	int32 mag_filter;
	int32 min_filter;
	int32 wrap_s; // default: 10497
	int32 wrap_t; // default: 10497
}
typedef GltfJson_Sampler;

struct GltfJson_Image
{
	String name;
	
	String mime_type;
	int32 buffer_view;
}
typedef GltfJson_Image;

struct GltfJson_Texture
{
	String name;
	
	int32 sampler;
	int32 source;
}
typedef GltfJson_Texture;

struct GltfJson_Accessor
{
	String type;
	int32 component_type;
	int32 count;
	int32 buffer_view;
	
	int16 max_count, min_count;
	float32 max[4];
	float32 min[4];
}
typedef GltfJson_Accessor;

struct GltfJson_BufferView
{
	int32 buffer;
	int32 byte_offset;
	int32 byte_length;
}
typedef GltfJson_BufferView;

struct GltfJson_Buffer
{
	const uint8* data;
	int32 byte_length;
}
typedef GltfJson_Buffer;

struct Engine_GltfJson
{
	int32 scene;
	
	struct
	{
		String generator;
		String version;
		String copyright;
	}
	asset;
	
	GltfJson_Scene* scenes;
	uintsize scene_count;
	
	GltfJson_Node* nodes;
	uintsize node_count;
	
	GltfJson_Material* materials;
	uintsize material_count;
	
	GltfJson_Mesh* meshes;
	uintsize mesh_count;
	
	GltfJson_Accessor* accessors;
	uintsize accessor_count;
	
	GltfJson_Sampler* samplers;
	uintsize sampler_count;
	
	GltfJson_Image* images;
	uintsize image_count;
	
	GltfJson_Texture* textures;
	uintsize texture_count;
	
	GltfJson_BufferView* buffer_views;
	uintsize buffer_view_count;
	
	GltfJson_Buffer* buffers;
	uintsize buffer_count;
}
typedef Engine_GltfJson;

//~ Functions
static inline const uint8*
ReadUint32(const uint8* begin, const uint8* end, uint32* out_value)
{
	if (begin + sizeof(uint32) > end)
		return NULL;
	
	Mem_Copy(out_value, begin, sizeof(uint32));
	return begin + sizeof(uint32);
}

static void
ParseGltfDataAsset(const Json_Value* value, Engine_GltfJson* out)
{
	out->asset.generator = StrNull;
	out->asset.version = StrNull;
	out->asset.copyright = StrNull;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("generator")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->asset.generator = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("version")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->asset.version = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("copyright")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->asset.version = Json_RawStringValue(&field_value);
		}
	}
}

static bool
ParseGltfDataNode(const Json_Value* value, GltfJson_Node* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->mesh = -1;
	
	bool matrix = false;
	vec3 translation = { 0.0f, 0.0f, 0.0f };
	versor rotation = GLM_QUAT_IDENTITY_INIT;
	vec3 scale = { 1.0f, 1.0f, 1.0f };
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("name")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->name = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("mesh")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->mesh = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("translation")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; Json_NextIndex(&index); ++i)
			{
				Json_Value value;
				Json_IndexValue(&index, &value);
				
				translation[i] = (float32)Json_NumberValueF64(&value);
			}
		}
		else if (String_Equals(field_name, Str("rotation")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; Json_NextIndex(&index); ++i)
			{
				Json_Value value;
				Json_IndexValue(&index, &value);
				
				rotation[i] = (float32)Json_NumberValueF64(&value);
			}
		}
		else if (String_Equals(field_name, Str("scale")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; Json_NextIndex(&index); ++i)
			{
				Json_Value value;
				Json_IndexValue(&index, &value);
				
				scale[i] = (float32)Json_NumberValueF64(&value);
			}
		}
		else if (String_Equals(field_name, Str("matrix")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			matrix = true;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; Json_NextIndex(&index); ++i)
			{
				Json_Value value;
				Json_IndexValue(&index, &value);
				
				((float32*)out->transform)[i] = (float32)Json_NumberValueF64(&value);
			}
		}
	}
	
	if (!matrix)
	{
		glm_mat4_identity(out->transform);
		glm_translate(out->transform, translation);
		
		mat4 rot;
		glm_quat_mat4(rotation, rot);
		glm_mat4_mul(out->transform, rot, out->transform);
		
		glm_scale(out->transform, scale);
	}
	
	return true;
}

static bool
ParseGltfDataNodes(const Json_Value* value, Engine_GltfJson* out)
{
	if (value->kind != Json_ValueKind_Array)
		return false;
	
	uintsize length = Json_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->nodes = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Node) * length);
	out->node_count = length;
	
	int32 i = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); ++i)
	{
		Json_Value index_value;
		Json_IndexValue(&index, &index_value);
		
		if (!ParseGltfDataNode(&index_value, &out->nodes[i]))
			return false;
	}
	
	return true;
}

static bool
ParseGltfDataScene(const Json_Value* value, GltfJson_Scene* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->nodes = NULL;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("name")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->name = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("nodes")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			uintsize length = Json_ArrayLength(&field_value);
			out->nodes = Arena_Push(global_engine.temp_arena, sizeof(int32) * length);
			out->node_count = length;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; Json_NextIndex(&index); ++i)
			{
				Json_Value index_value;
				Json_IndexValue(&index, &index_value);
				
				if (index_value.kind != Json_ValueKind_Number)
					return false;
				
				out->nodes[i] = (int32)Json_NumberValueI64(&index_value);
			}
		}
	}
	
	return true;
}

static bool
ParseGltfDataScenes(const Json_Value* value, Engine_GltfJson* out)
{
	if (value->kind != Json_ValueKind_Array)
		return false;
	
	uintsize length = Json_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->scenes = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Scene) * length);
	out->scene_count = length;
	
	int32 i = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); ++i)
	{
		Json_Value index_value;
		Json_IndexValue(&index, &index_value);
		
		if (!ParseGltfDataScene(&index_value, &out->scenes[i]))
			return false;
	}
	
	return true;
}

static bool
ParseGltfDataTextureInfo(const Json_Value* value, GltfJson_TextureInfo* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->specified = true;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("index")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->index = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("texcoord")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->texcoord = (int32)Json_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
ParseGltfDataMetallicRoughness(const Json_Value* value, GltfJson_MetallicRoughness* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	// NOTE(ljre): default values already assigned
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("metallicFactor")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->metalic_factor = (float32)Json_NumberValueF64(&field_value);
		}
		else if (String_Equals(field_name, Str("roughnessFactor")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->roughness_factor = (float32)Json_NumberValueF64(&field_value);
		}
		else if (String_Equals(field_name, Str("baseColorTexture")))
		{
			if (!ParseGltfDataTextureInfo(&field_value, &out->base_color_texture))
				return false;
		}
		else if (String_Equals(field_name, Str("metallicRoughnessTexture")))
		{
			if (!ParseGltfDataTextureInfo(&field_value, &out->metallic_roughness_texture))
				return false;
		}
	}
	
	return true;
}

static bool
ParseGltfDataNormalTextureInfo(const Json_Value* value, GltfJson_NormalTextureInfo* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->specified = true;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("index")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->index = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("texcoord")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->texcoord = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("scale")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->scale = (float32)Json_NumberValueF64(&field_value);
		}
	}
	
	return true;
}

static bool
ParseGltfDataOcclusionTextureInfo(const Json_Value* value, GltfJson_OcclusionTextureInfo* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->specified = true;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("index")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->index = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("texcoord")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->texcoord = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("strength")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->strength = (float32)Json_NumberValueF64(&field_value);
		}
	}
	
	return true;
}

static bool
ParseGltfDataMaterial(const Json_Value* value, GltfJson_Material* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->pbr_metallic_roughness.base_color_factor[0] = 1.0f;
	out->pbr_metallic_roughness.base_color_factor[1] = 1.0f;
	out->pbr_metallic_roughness.base_color_factor[2] = 1.0f;
	out->pbr_metallic_roughness.base_color_factor[3] = 1.0f;
	out->pbr_metallic_roughness.metalic_factor = 1.0f;
	out->pbr_metallic_roughness.roughness_factor = 1.0f;
	out->pbr_metallic_roughness.base_color_texture.specified = false;
	out->pbr_metallic_roughness.base_color_texture.index = -1;
	out->pbr_metallic_roughness.base_color_texture.texcoord = 0;
	out->pbr_metallic_roughness.metallic_roughness_texture.specified = false;
	out->pbr_metallic_roughness.metallic_roughness_texture.index = -1;
	out->pbr_metallic_roughness.metallic_roughness_texture.texcoord = 0;
	out->normal_texture.specified = false;
	out->normal_texture.index = -1;
	out->normal_texture.texcoord = 0;
	out->normal_texture.scale = 1.0f;
	out->occlusion_texture.specified = false;
	out->occlusion_texture.index = -1;
	out->occlusion_texture.texcoord = 0;
	out->occlusion_texture.strength = 1.0f;
	out->emissive_texture.specified = false;
	out->emissive_texture.index = -1;
	out->emissive_texture.texcoord = 0;
	out->emissive_factor[0] = 0.0f;
	out->emissive_factor[1] = 0.0f;
	out->emissive_factor[2] = 0.0f;
	out->alpha_mode = Str("OPAQUE");
	out->alpha_cutoff = 0.5f;
	out->double_sided = false;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("name")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->name = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("pbrMetallicRoughness")))
		{
			if (!ParseGltfDataMetallicRoughness(&field_value, &out->pbr_metallic_roughness))
				return false;
		}
		else if (String_Equals(field_name, Str("normalTexture")))
		{
			if (!ParseGltfDataNormalTextureInfo(&field_value, &out->normal_texture))
				return false;
		}
		else if (String_Equals(field_name, Str("occlusionTexture")))
		{
			if (!ParseGltfDataOcclusionTextureInfo(&field_value, &out->occlusion_texture))
				return false;
		}
		else if (String_Equals(field_name, Str("emissiveTexture")))
		{
			if (!ParseGltfDataTextureInfo(&field_value, &out->emissive_texture))
				return false;
		}
		else if (String_Equals(field_name, Str("emissiveFactor")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; i < ArrayLength(out->emissive_factor) && Json_NextIndex(&index); ++i)
			{
				Json_Value value;
				Json_IndexValue(&index, &value);
				
				if (value.kind != Json_ValueKind_Number)
					return false;
				
				out->emissive_factor[i] = (float32)Json_NumberValueF64(&value);
			}
		}
		else if (String_Equals(field_name, Str("alphaMode")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->alpha_mode = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("alphaCutoff")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->alpha_cutoff = (float32)Json_NumberValueF64(&field_value);
		}
		else if (String_Equals(field_name, Str("doubleSided")))
		{
			if (field_value.kind == Json_ValueKind_Bool)
				out->double_sided = Json_BoolValue(&field_value);
		}
	}
	
	return true;
}

static bool
ParseGltfDataMaterials(const Json_Value* value, Engine_GltfJson* out)
{
	if (value->kind != Json_ValueKind_Array)
		return false;
	
	uintsize length = Json_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->materials = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Material) * length);
	out->material_count = length;
	
	int32 i = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); ++i)
	{
		Json_Value index_value;
		Json_IndexValue(&index, &index_value);
		
		if (!ParseGltfDataMaterial(&index_value, &out->materials[i]))
			return false;
	}
	
	return true;
}

static bool
ParseGltfDataPrimitive(const Json_Value* value, GltfJson_Primitive* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->mode = 4;
	out->indices = -1;
	out->attributes.position = -1;
	out->attributes.normal = -1;
	out->attributes.texcoord_0 = -1;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("indices")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->indices = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("mode")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->mode = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("attributes")))
		{
			if (field_value.kind != Json_ValueKind_Object)
				return false;
			
			for (Json_Field attrib_field = { &field_value }; Json_NextField(&attrib_field); )
			{
				String attrib_field_name = Json_RawFieldName(&attrib_field);
				Json_Value attrib_field_value;
				Json_FieldValue(&attrib_field, &attrib_field_value);
				
				if (String_Equals(attrib_field_name, Str("POSITION")))
				{
					if (attrib_field_value.kind == Json_ValueKind_Number)
						out->attributes.position = (int32)Json_NumberValueI64(&attrib_field_value);
				}
				else if (String_Equals(attrib_field_name, Str("NORMAL")))
				{
					if (attrib_field_value.kind == Json_ValueKind_Number)
						out->attributes.normal = (int32)Json_NumberValueI64(&attrib_field_value);
				}
				else if (String_Equals(attrib_field_name, Str("TANGENT")))
				{
					if (attrib_field_value.kind == Json_ValueKind_Number)
						out->attributes.tangent = (int32)Json_NumberValueI64(&attrib_field_value);
				}
				else if (String_Equals(attrib_field_name, Str("TEXCOORD_0")))
				{
					if (attrib_field_value.kind == Json_ValueKind_Number)
						out->attributes.texcoord_0 = (int32)Json_NumberValueI64(&attrib_field_value);
				}
			}
		}
	}
	
	return true;
}

static bool
ParseGltfDataMesh(const Json_Value* value, GltfJson_Mesh* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->primitives = NULL;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("name")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->name = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("primitives")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			uintsize length = Json_ArrayLength(&field_value);
			out->primitives = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Primitive) * length);
			out->primitive_count = length;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; Json_NextIndex(&index); ++i)
			{
				Json_Value index_value;
				Json_IndexValue(&index, &index_value);
				
				if (!ParseGltfDataPrimitive(&index_value, &out->primitives[i]))
					return false;
			}
		}
	}
	
	return true;
}

static bool
ParseGltfDataMeshes(const Json_Value* value, Engine_GltfJson* out)
{
	if (value->kind != Json_ValueKind_Array)
		return false;
	
	uintsize length = Json_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->meshes = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Mesh) * length);
	out->mesh_count = length;
	
	int32 i = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); ++i)
	{
		Json_Value index_value;
		Json_IndexValue(&index, &index_value);
		
		if (!ParseGltfDataMesh(&index_value, &out->meshes[i]))
			return false;
	}
	
	return true;
}

static bool
ParseGltfDataAccessor(const Json_Value* value, GltfJson_Accessor* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->type = StrNull;
	out->component_type = -1;
	out->count = -1;
	out->buffer_view = -1;
	out->max_count = 0;
	out->min_count = 0;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("type")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->type = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("componentType")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->component_type = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("count")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->count = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("bufferView")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->buffer_view = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("max")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			uintsize length = Json_ArrayLength(&field_value);
			if (length > 4)
				return false;
			
			out->max_count = (int16)length;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; Json_NextIndex(&index); ++i)
			{
				Json_Value index_value;
				Json_IndexValue(&index, &index_value);
				
				if (index_value.kind != Json_ValueKind_Number)
					return false;
				
				out->max[i] = (float32)Json_NumberValueF64(&index_value);
			}
		}
		else if (String_Equals(field_name, Str("min")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			uintsize length = Json_ArrayLength(&field_value);
			if (length > 4)
				return false;
			
			out->min_count = (int16)length;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; Json_NextIndex(&index); ++i)
			{
				Json_Value index_value;
				Json_IndexValue(&index, &index_value);
				
				if (index_value.kind != Json_ValueKind_Number)
					return false;
				
				out->min[i] = (float32)Json_NumberValueF64(&index_value);
			}
		}
	}
	
	return true;
}

static bool
ParseGltfDataAccessors(const Json_Value* value, Engine_GltfJson* out)
{
	if (value->kind != Json_ValueKind_Array)
		return false;
	
	uintsize length = Json_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->accessors = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Accessor) * length);
	out->accessor_count = length;
	
	int32 i = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); ++i)
	{
		Json_Value index_value;
		Json_IndexValue(&index, &index_value);
		
		if (!ParseGltfDataAccessor(&index_value, &out->accessors[i]))
			return false;
	}
	
	return true;
}

static bool
ParseGltfDataSampler(const Json_Value* value, GltfJson_Sampler* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->mag_filter = -1;
	out->min_filter = -1;
	out->wrap_s = 10497;
	out->wrap_t = 10497;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("name")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->name = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("magFilter")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->mag_filter = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("minFilter")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->min_filter = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("wrapS")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->wrap_s = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("wrapT")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->wrap_t = (int32)Json_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
ParseGltfDataSamplers(const Json_Value* value, Engine_GltfJson* out)
{
	if (value->kind != Json_ValueKind_Array)
		return false;
	
	uintsize length = Json_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->samplers = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Sampler) * length);
	out->sampler_count = length;
	
	int32 i = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); ++i)
	{
		Json_Value index_value;
		Json_IndexValue(&index, &index_value);
		
		if (!ParseGltfDataSampler(&index_value, &out->samplers[i]))
			return false;
	}
	
	return true;
}

static bool
ParseGltfDataImage(const Json_Value* value, GltfJson_Image* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->mime_type = StrNull;
	out->buffer_view = -1;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("name")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->name = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("mimeType")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->mime_type = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("bufferView")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->buffer_view = (int32)Json_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
ParseGltfDataImages(const Json_Value* value, Engine_GltfJson* out)
{
	if (value->kind != Json_ValueKind_Array)
		return false;
	
	uintsize length = Json_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->images = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Image) * length);
	out->image_count = length;
	
	int32 i = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); ++i)
	{
		Json_Value index_value;
		Json_IndexValue(&index, &index_value);
		
		if (!ParseGltfDataImage(&index_value, &out->images[i]))
			return false;
	}
	
	return true;
}

static bool
ParseGltfDataTexture(const Json_Value* value, GltfJson_Texture* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->sampler = -1;
	out->source = -1;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("name")))
		{
			if (field_value.kind == Json_ValueKind_String)
				out->name = Json_RawStringValue(&field_value);
		}
		else if (String_Equals(field_name, Str("sampler")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->sampler = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("source")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->source = (int32)Json_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
ParseGltfDataTextures(const Json_Value* value, Engine_GltfJson* out)
{
	if (value->kind != Json_ValueKind_Array)
		return false;
	
	uintsize length = Json_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->textures = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Texture) * length);
	out->texture_count = length;
	
	int32 i = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); ++i)
	{
		Json_Value index_value;
		Json_IndexValue(&index, &index_value);
		
		if (!ParseGltfDataTexture(&index_value, &out->textures[i]))
			return false;
	}
	
	return true;
}

static bool
ParseGltfDataBufferView(const Json_Value* value, GltfJson_BufferView* out)
{
	if (value->kind != Json_ValueKind_Object)
		return false;
	
	out->buffer = -1;
	out->byte_offset = -1;
	out->byte_length = -1;
	
	for (Json_Field field = { value }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("buffer")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->buffer = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("byteOffset")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->byte_offset = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("byteLength")))
		{
			if (field_value.kind == Json_ValueKind_Number)
				out->byte_length = (int32)Json_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
ParseGltfDataBufferViews(const Json_Value* value, Engine_GltfJson* out)
{
	if (value->kind != Json_ValueKind_Array)
		return false;
	
	uintsize length = Json_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->buffer_views = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_BufferView) * length);
	out->buffer_view_count = length;
	
	int32 i = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); ++i)
	{
		Json_Value index_value;
		Json_IndexValue(&index, &index_value);
		
		if (!ParseGltfDataBufferView(&index_value, &out->buffer_views[i]))
			return false;
	}
	
	return true;
}

static bool
ParseGltfData(const uint8* json_begin, const uint8* json_end,
	const uint8* binary_begin, const uint8* binary_end, Engine_GltfJson* out)
{
	Json_Value json;
	Json_InitFromBufferRange(json_begin, json_end, &json);
	
	out->scene_count = 0;
	out->node_count = 0;
	out->material_count = 0;
	out->mesh_count = 0;
	out->accessor_count = 0;
	out->sampler_count = 0;
	out->image_count = 0;
	out->texture_count = 0;
	out->buffer_view_count = 0;
	out->buffer_count = 0;
	
	for (Json_Field field = { &json }; Json_NextField(&field); )
	{
		String field_name = Json_RawFieldName(&field);
		Json_Value field_value;
		Json_FieldValue(&field, &field_value);
		
		if (String_Equals(field_name, Str("asset")))
		{
			ParseGltfDataAsset(&field_value, out);
		}
		else if (String_Equals(field_name, Str("scene")))
		{
			if (field_value.kind != Json_ValueKind_Number)
				out->scene = -1;
			else
				out->scene = (int32)Json_NumberValueI64(&field_value);
		}
		else if (String_Equals(field_name, Str("scenes")))
		{
			if (!ParseGltfDataScenes(&field_value, out))
				return false;
		}
		else if (String_Equals(field_name, Str("nodes")))
		{
			if (!ParseGltfDataNodes(&field_value, out))
				return false;
		}
		else if (String_Equals(field_name, Str("materials")))
		{
			if (!ParseGltfDataMaterials(&field_value, out))
				return false;
		}
		else if (String_Equals(field_name, Str("meshes")))
		{
			if (!ParseGltfDataMeshes(&field_value, out))
				return false;
		}
		else if (String_Equals(field_name, Str("accessors")))
		{
			if (!ParseGltfDataAccessors(&field_value, out))
				return false;
		}
		else if (String_Equals(field_name, Str("samplers")))
		{
			if (!ParseGltfDataSamplers(&field_value, out))
				return false;
		}
		else if (String_Equals(field_name, Str("images")))
		{
			if (!ParseGltfDataImages(&field_value, out))
				return false;
		}
		else if (String_Equals(field_name, Str("textures")))
		{
			if (!ParseGltfDataTextures(&field_value, out))
				return false;
		}
		else if (String_Equals(field_name, Str("bufferViews")))
		{
			if (!ParseGltfDataBufferViews(&field_value, out))
				return false;
		}
		else if (String_Equals(field_name, Str("buffers")))
		{
			if (field_value.kind != Json_ValueKind_Array)
				return false;
			
			uintsize length = Json_ArrayLength(&field_value);
			if (length == 0)
				return false;
			
			out->buffers = Arena_Push(global_engine.temp_arena, sizeof(GltfJson_Buffer) * length);
			out->buffer_count = length;
			
			int32 i = 0;
			for (Json_ArrayIndex index = { &field_value }; Json_NextIndex(&index); ++i)
			{
				Json_Value index_value;
				Json_IndexValue(&index, &index_value);
				
				if (index_value.kind != Json_ValueKind_Object)
					return false;
				
				Json_Value buffer_byte_length;
				Json_FindFieldValue(&index_value, Str("byteLength"), &buffer_byte_length);
				
				if (buffer_byte_length.kind != Json_ValueKind_Number)
					return false;
				
				out->buffers[i].data = binary_begin;
				out->buffers[i].byte_length = (int32)Json_NumberValueI64(&buffer_byte_length);
				
				if (out->buffers[i].byte_length != binary_end - binary_begin)
					return false;
			}
		}
	}
	
	return true;
}

//~ Internal API
static bool
Engine_ParseGltf(const uint8* data, uintsize size, Engine_GltfJson* out)
{
	Trace();
	
	const uint8* begin = data;
	const uint8* end = begin + size;
	
	// NOTE(ljre): Parse Header
	if (size < 12)
		return false;
	
	uint32 magic, version, length;
	
	begin = ReadUint32(begin, end, &magic);
	begin = ReadUint32(begin, end, &version);
	begin = ReadUint32(begin, end, &length);
	
	if (length != size || magic != 0x46546C67 || version > 2)
		return false;
	
	const uint8* json_begin = NULL;
	const uint8* json_end = NULL;
	
	const uint8* binary_begin = NULL;
	const uint8* binary_end = NULL;
	
	while (begin + 8 < end)
	{
		uint32 chunk_length, chunk_type;
		
		begin = ReadUint32(begin, end, &chunk_length);
		begin = ReadUint32(begin, end, &chunk_type);
		
		// NOTE(ljre): Chunks should have alignment of 4 bytes
		chunk_length = AlignUp(chunk_length, 3u);
		
		if (begin + chunk_length > end)
			return false;
		
		switch (chunk_type)
		{
			case 0x4E4F534A: // JSON
			{
				json_begin = begin;
				json_end = json_begin + chunk_length;
			} break;
			
			case 0x004E4942: // BIN
			{
				binary_begin = begin;
				binary_end = binary_begin + chunk_length;
			} break;
		}
		
		begin += chunk_length;
	}
	
	if (!json_begin)
		return false;
	
	bool result = ParseGltfData(json_begin, json_end, binary_begin, binary_end, out);
	
	return result;
}
