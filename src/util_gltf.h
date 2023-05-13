#ifndef UTIL_GLTF_H
#define UTIL_GLTF_H

struct UGltf_JsonScene
{
	String name;
	int32* nodes;
	uintsize node_count;
}
typedef UGltf_JsonScene;

struct UGltf_JsonNode
{
	String name;
	int32 mesh;
	
	mat4 transform;
}
typedef UGltf_JsonNode;

struct UGltf_JsonTextureInfo
{
	bool32 specified;
	
	int32 index;
	int32 texcoord; // default: 0
}
typedef UGltf_JsonTextureInfo;

struct UGltf_JsonMetallicRoughness
{
	vec4 base_color_factor; // default: { 1, 1, 1, 1 }
	
	float32 metalic_factor; // detault: 1
	float32 roughness_factor; // default: 1
	
	UGltf_JsonTextureInfo base_color_texture;
	UGltf_JsonTextureInfo metallic_roughness_texture;
}
typedef UGltf_JsonMetallicRoughness;

struct UGltf_JsonNormalTextureInfo
{
	bool32 specified;
	
	int32 index;
	int32 texcoord; // default: 0
	float32 scale; // default: 1
}
typedef UGltf_JsonNormalTextureInfo;

struct UGltf_JsonOcclusionTextureInfo
{
	bool32 specified;
	
	int32 index;
	int32 texcoord; // default: 0
	float32 strength; // default: 1
}
typedef UGltf_JsonOcclusionTextureInfo;

struct UGltf_JsonMaterial
{
	String name;
	UGltf_JsonMetallicRoughness pbr_metallic_roughness;
	UGltf_JsonNormalTextureInfo normal_texture;
	UGltf_JsonOcclusionTextureInfo occlusion_texture;
	UGltf_JsonTextureInfo emissive_texture;
	
	vec3 emissive_factor; // default: { 0, 0, 0 }
	String alpha_mode; // default: "OPAQUE"
	float32 alpha_cutoff; // default: 0.5
	bool32 double_sided; // default: false
}
typedef UGltf_JsonMaterial;

struct UGltf_JsonPrimitive
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
typedef UGltf_JsonPrimitive;

struct UGltf_JsonMesh
{
	String name;
	UGltf_JsonPrimitive* primitives;
	uintsize primitive_count;
}
typedef UGltf_JsonMesh;

struct UGltf_JsonSampler
{
	String name;
	
	int32 mag_filter;
	int32 min_filter;
	int32 wrap_s; // default: 10497
	int32 wrap_t; // default: 10497
}
typedef UGltf_JsonSampler;

struct UGltf_JsonImage
{
	String name;
	
	String mime_type;
	int32 buffer_view;
}
typedef UGltf_JsonImage;

struct UGltf_JsonTexture
{
	String name;
	
	int32 sampler;
	int32 source;
}
typedef UGltf_JsonTexture;

struct UGltf_JsonAccessor
{
	String type;
	int32 component_type;
	int32 count;
	int32 buffer_view;
	
	int16 max_count, min_count;
	float32 max[4];
	float32 min[4];
}
typedef UGltf_JsonAccessor;

struct UGltf_JsonBufferView
{
	int32 buffer;
	int32 byte_offset;
	int32 byte_length;
}
typedef UGltf_JsonBufferView;

struct UGltf_JsonBuffer
{
	const uint8* data;
	int32 byte_length;
}
typedef UGltf_JsonBuffer;

struct UGltf_JsonRoot
{
	int32 scene;
	
	struct
	{
		String generator;
		String version;
		String copyright;
	}
	asset;
	
	UGltf_JsonScene* scenes;
	uintsize scene_count;
	
	UGltf_JsonNode* nodes;
	uintsize node_count;
	
	UGltf_JsonMaterial* materials;
	uintsize material_count;
	
	UGltf_JsonMesh* meshes;
	uintsize mesh_count;
	
	UGltf_JsonAccessor* accessors;
	uintsize accessor_count;
	
	UGltf_JsonSampler* samplers;
	uintsize sampler_count;
	
	UGltf_JsonImage* images;
	uintsize image_count;
	
	UGltf_JsonTexture* textures;
	uintsize texture_count;
	
	UGltf_JsonBufferView* buffer_views;
	uintsize buffer_view_count;
	
	UGltf_JsonBuffer* buffers;
	uintsize buffer_count;
}
typedef UGltf_JsonRoot;

//~ Functions
static inline const uint8*
UGltf_ReadUint32_(const uint8* begin, const uint8* end, uint32* out_value)
{
	if (begin + sizeof(uint32) > end)
		return NULL;
	
	*out_value = *(uint32*)begin;
	return begin + sizeof(uint32);
}

static void
UGltf_ParseDataAsset_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	out->asset.generator = StrNull;
	out->asset.version = StrNull;
	out->asset.copyright = StrNull;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("generator")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->asset.generator = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("version")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->asset.version = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("copyright")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->asset.version = UJson_RawStringValue(&field_value);
		}
	}
}

static bool
UGltf_ParseDataNode_(Arena* arena, const UJson_Value* value, UGltf_JsonNode* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->mesh = -1;
	
	bool matrix = false;
	vec3 translation = { 0.0f, 0.0f, 0.0f };
	versor rotation = GLM_QUAT_IDENTITY_INIT;
	vec3 scale = { 1.0f, 1.0f, 1.0f };
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("name")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->name = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("mesh")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->mesh = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("translation")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; UJson_NextIndex(&index); ++i)
			{
				UJson_Value value;
				UJson_IndexValue(&index, &value);
				
				translation[i] = (float32)UJson_NumberValueF64(&value);
			}
		}
		else if (StringEquals(field_name, Str("rotation")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; UJson_NextIndex(&index); ++i)
			{
				UJson_Value value;
				UJson_IndexValue(&index, &value);
				
				rotation[i] = (float32)UJson_NumberValueF64(&value);
			}
		}
		else if (StringEquals(field_name, Str("scale")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; UJson_NextIndex(&index); ++i)
			{
				UJson_Value value;
				UJson_IndexValue(&index, &value);
				
				scale[i] = (float32)UJson_NumberValueF64(&value);
			}
		}
		else if (StringEquals(field_name, Str("matrix")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			matrix = true;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; UJson_NextIndex(&index); ++i)
			{
				UJson_Value value;
				UJson_IndexValue(&index, &value);
				
				((float32*)out->transform)[i] = (float32)UJson_NumberValueF64(&value);
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
UGltf_ParseDataNodes_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	if (value->kind != UJson_ValueKind_Array)
		return false;
	
	uintsize length = UJson_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->nodes = ArenaPush(arena, sizeof(UGltf_JsonNode) * length);
	out->node_count = length;
	
	int32 i = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); ++i)
	{
		UJson_Value index_value;
		UJson_IndexValue(&index, &index_value);
		
		if (!UGltf_ParseDataNode_(arena, &index_value, &out->nodes[i]))
			return false;
	}
	
	return true;
}

static bool
UGltf_ParseDataScene_(Arena* arena, const UJson_Value* value, UGltf_JsonScene* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->nodes = NULL;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("name")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->name = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("nodes")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			uintsize length = UJson_ArrayLength(&field_value);
			out->nodes = ArenaPush(arena, sizeof(int32) * length);
			out->node_count = length;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; UJson_NextIndex(&index); ++i)
			{
				UJson_Value index_value;
				UJson_IndexValue(&index, &index_value);
				
				if (index_value.kind != UJson_ValueKind_Number)
					return false;
				
				out->nodes[i] = (int32)UJson_NumberValueI64(&index_value);
			}
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataScenes_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	if (value->kind != UJson_ValueKind_Array)
		return false;
	
	uintsize length = UJson_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->scenes = ArenaPush(arena, sizeof(UGltf_JsonScene) * length);
	out->scene_count = length;
	
	int32 i = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); ++i)
	{
		UJson_Value index_value;
		UJson_IndexValue(&index, &index_value);
		
		if (!UGltf_ParseDataScene_(arena, &index_value, &out->scenes[i]))
			return false;
	}
	
	return true;
}

static bool
UGltf_ParseDataTextureInfo_(Arena* arena, const UJson_Value* value, UGltf_JsonTextureInfo* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->specified = true;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("index")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->index = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("texcoord")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->texcoord = (int32)UJson_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataMetallicRoughness_(Arena* arena, const UJson_Value* value, UGltf_JsonMetallicRoughness* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	// NOTE(ljre): default values already assigned
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("metallicFactor")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->metalic_factor = (float32)UJson_NumberValueF64(&field_value);
		}
		else if (StringEquals(field_name, Str("roughnessFactor")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->roughness_factor = (float32)UJson_NumberValueF64(&field_value);
		}
		else if (StringEquals(field_name, Str("baseColorTexture")))
		{
			if (!UGltf_ParseDataTextureInfo_(arena, &field_value, &out->base_color_texture))
				return false;
		}
		else if (StringEquals(field_name, Str("metallicRoughnessTexture")))
		{
			if (!UGltf_ParseDataTextureInfo_(arena, &field_value, &out->metallic_roughness_texture))
				return false;
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataNormalTextureInfo_(Arena* arena, const UJson_Value* value, UGltf_JsonNormalTextureInfo* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->specified = true;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("index")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->index = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("texcoord")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->texcoord = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("scale")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->scale = (float32)UJson_NumberValueF64(&field_value);
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataOcclusionTextureInfo_(Arena* arena, const UJson_Value* value, UGltf_JsonOcclusionTextureInfo* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->specified = true;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("index")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->index = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("texcoord")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->texcoord = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("strength")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->strength = (float32)UJson_NumberValueF64(&field_value);
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataMaterial_(Arena* arena, const UJson_Value* value, UGltf_JsonMaterial* out)
{
	if (value->kind != UJson_ValueKind_Object)
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
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("name")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->name = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("pbrMetallicRoughness")))
		{
			if (!UGltf_ParseDataMetallicRoughness_(arena, &field_value, &out->pbr_metallic_roughness))
				return false;
		}
		else if (StringEquals(field_name, Str("normalTexture")))
		{
			if (!UGltf_ParseDataNormalTextureInfo_(arena, &field_value, &out->normal_texture))
				return false;
		}
		else if (StringEquals(field_name, Str("occlusionTexture")))
		{
			if (!UGltf_ParseDataOcclusionTextureInfo_(arena, &field_value, &out->occlusion_texture))
				return false;
		}
		else if (StringEquals(field_name, Str("emissiveTexture")))
		{
			if (!UGltf_ParseDataTextureInfo_(arena, &field_value, &out->emissive_texture))
				return false;
		}
		else if (StringEquals(field_name, Str("emissiveFactor")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; i < ArrayLength(out->emissive_factor) && UJson_NextIndex(&index); ++i)
			{
				UJson_Value value;
				UJson_IndexValue(&index, &value);
				
				if (value.kind != UJson_ValueKind_Number)
					return false;
				
				out->emissive_factor[i] = (float32)UJson_NumberValueF64(&value);
			}
		}
		else if (StringEquals(field_name, Str("alphaMode")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->alpha_mode = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("alphaCutoff")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->alpha_cutoff = (float32)UJson_NumberValueF64(&field_value);
		}
		else if (StringEquals(field_name, Str("doubleSided")))
		{
			if (field_value.kind == UJson_ValueKind_Bool)
				out->double_sided = UJson_BoolValue(&field_value);
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataMaterials_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	if (value->kind != UJson_ValueKind_Array)
		return false;
	
	uintsize length = UJson_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->materials = ArenaPush(arena, sizeof(UGltf_JsonMaterial) * length);
	out->material_count = length;
	
	int32 i = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); ++i)
	{
		UJson_Value index_value;
		UJson_IndexValue(&index, &index_value);
		
		if (!UGltf_ParseDataMaterial_(arena, &index_value, &out->materials[i]))
			return false;
	}
	
	return true;
}

static bool
UGltf_ParseDataPrimitive_(Arena* arena, const UJson_Value* value, UGltf_JsonPrimitive* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->mode = 4;
	out->indices = -1;
	out->attributes.position = -1;
	out->attributes.normal = -1;
	out->attributes.texcoord_0 = -1;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("indices")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->indices = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("mode")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->mode = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("attributes")))
		{
			if (field_value.kind != UJson_ValueKind_Object)
				return false;
			
			for (UJson_Field attrib_field = { &field_value }; UJson_NextField(&attrib_field); )
			{
				String attrib_field_name = UJson_RawFieldName(&attrib_field);
				UJson_Value attrib_field_value;
				UJson_FieldValue(&attrib_field, &attrib_field_value);
				
				if (StringEquals(attrib_field_name, Str("POSITION")))
				{
					if (attrib_field_value.kind == UJson_ValueKind_Number)
						out->attributes.position = (int32)UJson_NumberValueI64(&attrib_field_value);
				}
				else if (StringEquals(attrib_field_name, Str("NORMAL")))
				{
					if (attrib_field_value.kind == UJson_ValueKind_Number)
						out->attributes.normal = (int32)UJson_NumberValueI64(&attrib_field_value);
				}
				else if (StringEquals(attrib_field_name, Str("TANGENT")))
				{
					if (attrib_field_value.kind == UJson_ValueKind_Number)
						out->attributes.tangent = (int32)UJson_NumberValueI64(&attrib_field_value);
				}
				else if (StringEquals(attrib_field_name, Str("TEXCOORD_0")))
				{
					if (attrib_field_value.kind == UJson_ValueKind_Number)
						out->attributes.texcoord_0 = (int32)UJson_NumberValueI64(&attrib_field_value);
				}
			}
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataMesh_(Arena* arena, const UJson_Value* value, UGltf_JsonMesh* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->primitives = NULL;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("name")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->name = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("primitives")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			uintsize length = UJson_ArrayLength(&field_value);
			out->primitives = ArenaPush(arena, sizeof(UGltf_JsonPrimitive) * length);
			out->primitive_count = length;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; UJson_NextIndex(&index); ++i)
			{
				UJson_Value index_value;
				UJson_IndexValue(&index, &index_value);
				
				if (!UGltf_ParseDataPrimitive_(arena, &index_value, &out->primitives[i]))
					return false;
			}
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataMeshes_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	if (value->kind != UJson_ValueKind_Array)
		return false;
	
	uintsize length = UJson_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->meshes = ArenaPush(arena, sizeof(UGltf_JsonMesh) * length);
	out->mesh_count = length;
	
	int32 i = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); ++i)
	{
		UJson_Value index_value;
		UJson_IndexValue(&index, &index_value);
		
		if (!UGltf_ParseDataMesh_(arena, &index_value, &out->meshes[i]))
			return false;
	}
	
	return true;
}

static bool
UGltf_ParseDataAccessor_(Arena* arena, const UJson_Value* value, UGltf_JsonAccessor* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->type = StrNull;
	out->component_type = -1;
	out->count = -1;
	out->buffer_view = -1;
	out->max_count = 0;
	out->min_count = 0;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("type")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->type = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("componentType")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->component_type = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("count")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->count = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("bufferView")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->buffer_view = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("max")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			uintsize length = UJson_ArrayLength(&field_value);
			if (length > 4)
				return false;
			
			out->max_count = (int16)length;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; UJson_NextIndex(&index); ++i)
			{
				UJson_Value index_value;
				UJson_IndexValue(&index, &index_value);
				
				if (index_value.kind != UJson_ValueKind_Number)
					return false;
				
				out->max[i] = (float32)UJson_NumberValueF64(&index_value);
			}
		}
		else if (StringEquals(field_name, Str("min")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			uintsize length = UJson_ArrayLength(&field_value);
			if (length > 4)
				return false;
			
			out->min_count = (int16)length;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; UJson_NextIndex(&index); ++i)
			{
				UJson_Value index_value;
				UJson_IndexValue(&index, &index_value);
				
				if (index_value.kind != UJson_ValueKind_Number)
					return false;
				
				out->min[i] = (float32)UJson_NumberValueF64(&index_value);
			}
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataAccessors_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	if (value->kind != UJson_ValueKind_Array)
		return false;
	
	uintsize length = UJson_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->accessors = ArenaPush(arena, sizeof(UGltf_JsonAccessor) * length);
	out->accessor_count = length;
	
	int32 i = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); ++i)
	{
		UJson_Value index_value;
		UJson_IndexValue(&index, &index_value);
		
		if (!UGltf_ParseDataAccessor_(arena, &index_value, &out->accessors[i]))
			return false;
	}
	
	return true;
}

static bool
UGltf_ParseDataSampler_(Arena* arena, const UJson_Value* value, UGltf_JsonSampler* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->mag_filter = -1;
	out->min_filter = -1;
	out->wrap_s = 10497;
	out->wrap_t = 10497;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("name")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->name = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("magFilter")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->mag_filter = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("minFilter")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->min_filter = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("wrapS")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->wrap_s = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("wrapT")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->wrap_t = (int32)UJson_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataSamplers_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	if (value->kind != UJson_ValueKind_Array)
		return false;
	
	uintsize length = UJson_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->samplers = ArenaPush(arena, sizeof(UGltf_JsonSampler) * length);
	out->sampler_count = length;
	
	int32 i = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); ++i)
	{
		UJson_Value index_value;
		UJson_IndexValue(&index, &index_value);
		
		if (!UGltf_ParseDataSampler_(arena, &index_value, &out->samplers[i]))
			return false;
	}
	
	return true;
}

static bool
UGltf_ParseDataImage_(Arena* arena, const UJson_Value* value, UGltf_JsonImage* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->mime_type = StrNull;
	out->buffer_view = -1;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("name")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->name = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("mimeType")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->mime_type = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("bufferView")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->buffer_view = (int32)UJson_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataImages_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	if (value->kind != UJson_ValueKind_Array)
		return false;
	
	uintsize length = UJson_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->images = ArenaPush(arena, sizeof(UGltf_JsonImage) * length);
	out->image_count = length;
	
	int32 i = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); ++i)
	{
		UJson_Value index_value;
		UJson_IndexValue(&index, &index_value);
		
		if (!UGltf_ParseDataImage_(arena, &index_value, &out->images[i]))
			return false;
	}
	
	return true;
}

static bool
UGltf_ParseDataTexture_(Arena* arena, const UJson_Value* value, UGltf_JsonTexture* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->name = StrNull;
	out->sampler = -1;
	out->source = -1;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("name")))
		{
			if (field_value.kind == UJson_ValueKind_String)
				out->name = UJson_RawStringValue(&field_value);
		}
		else if (StringEquals(field_name, Str("sampler")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->sampler = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("source")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->source = (int32)UJson_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataTextures_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	if (value->kind != UJson_ValueKind_Array)
		return false;
	
	uintsize length = UJson_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->textures = ArenaPush(arena, sizeof(UGltf_JsonTexture) * length);
	out->texture_count = length;
	
	int32 i = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); ++i)
	{
		UJson_Value index_value;
		UJson_IndexValue(&index, &index_value);
		
		if (!UGltf_ParseDataTexture_(arena, &index_value, &out->textures[i]))
			return false;
	}
	
	return true;
}

static bool
UGltf_ParseDataBufferView_(Arena* arena, const UJson_Value* value, UGltf_JsonBufferView* out)
{
	if (value->kind != UJson_ValueKind_Object)
		return false;
	
	out->buffer = -1;
	out->byte_offset = -1;
	out->byte_length = -1;
	
	for (UJson_Field field = { value }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("buffer")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->buffer = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("byteOffset")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->byte_offset = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("byteLength")))
		{
			if (field_value.kind == UJson_ValueKind_Number)
				out->byte_length = (int32)UJson_NumberValueI64(&field_value);
		}
	}
	
	return true;
}

static bool
UGltf_ParseDataBufferViews_(Arena* arena, const UJson_Value* value, UGltf_JsonRoot* out)
{
	if (value->kind != UJson_ValueKind_Array)
		return false;
	
	uintsize length = UJson_ArrayLength(value);
	if (length == 0)
		return false;
	
	out->buffer_views = ArenaPush(arena, sizeof(UGltf_JsonBufferView) * length);
	out->buffer_view_count = length;
	
	int32 i = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); ++i)
	{
		UJson_Value index_value;
		UJson_IndexValue(&index, &index_value);
		
		if (!UGltf_ParseDataBufferView_(arena, &index_value, &out->buffer_views[i]))
			return false;
	}
	
	return true;
}

static bool
UGltf_ParseData_(Arena* arena, const uint8* json_begin, const uint8* json_end,
	const uint8* binary_begin, const uint8* binary_end, UGltf_JsonRoot* out)
{
	UJson_Value json;
	UJson_InitFromBufferRange(json_begin, json_end, &json);
	
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
	
	for (UJson_Field field = { &json }; UJson_NextField(&field); )
	{
		String field_name = UJson_RawFieldName(&field);
		UJson_Value field_value;
		UJson_FieldValue(&field, &field_value);
		
		if (StringEquals(field_name, Str("asset")))
		{
			UGltf_ParseDataAsset_(arena, &field_value, out);
		}
		else if (StringEquals(field_name, Str("scene")))
		{
			if (field_value.kind != UJson_ValueKind_Number)
				out->scene = -1;
			else
				out->scene = (int32)UJson_NumberValueI64(&field_value);
		}
		else if (StringEquals(field_name, Str("scenes")))
		{
			if (!UGltf_ParseDataScenes_(arena, &field_value, out))
				return false;
		}
		else if (StringEquals(field_name, Str("nodes")))
		{
			if (!UGltf_ParseDataNodes_(arena, &field_value, out))
				return false;
		}
		else if (StringEquals(field_name, Str("materials")))
		{
			if (!UGltf_ParseDataMaterials_(arena, &field_value, out))
				return false;
		}
		else if (StringEquals(field_name, Str("meshes")))
		{
			if (!UGltf_ParseDataMeshes_(arena, &field_value, out))
				return false;
		}
		else if (StringEquals(field_name, Str("accessors")))
		{
			if (!UGltf_ParseDataAccessors_(arena, &field_value, out))
				return false;
		}
		else if (StringEquals(field_name, Str("samplers")))
		{
			if (!UGltf_ParseDataSamplers_(arena, &field_value, out))
				return false;
		}
		else if (StringEquals(field_name, Str("images")))
		{
			if (!UGltf_ParseDataImages_(arena, &field_value, out))
				return false;
		}
		else if (StringEquals(field_name, Str("textures")))
		{
			if (!UGltf_ParseDataTextures_(arena, &field_value, out))
				return false;
		}
		else if (StringEquals(field_name, Str("bufferViews")))
		{
			if (!UGltf_ParseDataBufferViews_(arena, &field_value, out))
				return false;
		}
		else if (StringEquals(field_name, Str("buffers")))
		{
			if (field_value.kind != UJson_ValueKind_Array)
				return false;
			
			uintsize length = UJson_ArrayLength(&field_value);
			if (length == 0)
				return false;
			
			out->buffers = ArenaPush(arena, sizeof(UGltf_JsonBuffer) * length);
			out->buffer_count = length;
			
			int32 i = 0;
			for (UJson_ArrayIndex index = { &field_value }; UJson_NextIndex(&index); ++i)
			{
				UJson_Value index_value;
				UJson_IndexValue(&index, &index_value);
				
				if (index_value.kind != UJson_ValueKind_Object)
					return false;
				
				UJson_Value buffer_byte_length;
				UJson_FindFieldValue(&index_value, Str("byteLength"), &buffer_byte_length);
				
				if (buffer_byte_length.kind != UJson_ValueKind_Number)
					return false;
				
				out->buffers[i].data = binary_begin;
				out->buffers[i].byte_length = (int32)UJson_NumberValueI64(&buffer_byte_length);
				
				if (out->buffers[i].byte_length != binary_end - binary_begin)
					return false;
			}
		}
	}
	
	return true;
}

//~ API
static bool
UGltf_Parse(const uint8* data, uintsize size, Arena* arena, UGltf_JsonRoot* out)
{
	Trace();
	
	const uint8* begin = data;
	const uint8* end = begin + size;
	
	// NOTE(ljre): Parse Header
	if (size < 12)
		return false;
	
	uint32 magic, version, length;
	
	begin = UGltf_ReadUint32_(begin, end, &magic);
	begin = UGltf_ReadUint32_(begin, end, &version);
	begin = UGltf_ReadUint32_(begin, end, &length);
	
	if (length != size || magic != 0x46546C67 || version > 2)
		return false;
	
	const uint8* json_begin = NULL;
	const uint8* json_end = NULL;
	
	const uint8* binary_begin = NULL;
	const uint8* binary_end = NULL;
	
	while (begin + 8 < end)
	{
		uint32 chunk_length, chunk_type;
		
		begin = UGltf_ReadUint32_(begin, end, &chunk_length);
		begin = UGltf_ReadUint32_(begin, end, &chunk_type);
		
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
	
	bool result = UGltf_ParseData_(arena, json_begin, json_end, binary_begin, binary_end, out);
	
	return result;
}

#endif //UTIL_GLTF_H
