struct GltfJson_Scene
{
    String name;
    int32* nodes;
    uintsize node_count;
} typedef GltfJson_Scene;

struct GltfJson_Node
{
    String name;
    int32 mesh;
    
    mat4 transform;
} typedef GltfJson_Node;

struct GltfJson_Primitive
{
    // NOTE(ljre): -1 for "undefined"
    int32 indices;
    int32 mode; // default: 4
    
    struct {
        int32 position;
        int32 normal;
        int32 texcoord_0;
    } attributes;
    
} typedef GltfJson_Primitive;

struct GltfJson_Mesh
{
    String name;
    GltfJson_Primitive* primitives;
    uintsize primitive_count;
} typedef GltfJson_Mesh;

struct GltfJson_Accessor
{
    String type;
    int32 component_type;
    int32 count;
    int32 buffer_view;
    
    int16 max_count, min_count;
    float32 max[4];
    float32 min[4];
} typedef GltfJson_Accessor;

struct GltfJson_BufferView
{
    int32 buffer;
    int32 byte_offset;
    int32 byte_length;
} typedef GltfJson_BufferView;

struct GltfJson_Buffer
{
    const uint8* data;
    int32 byte_length;
} typedef GltfJson_Buffer;

struct Engine_GltfJson
{
    int32 scene;
    
    struct {
        String generator;
        String version;
        String copyright;
    } asset;
    
    GltfJson_Scene* scenes;
    uintsize scene_count;
    
    GltfJson_Node* nodes;
    uintsize node_count;
    
    GltfJson_Mesh* meshes;
    uintsize mesh_count;
    
    GltfJson_Accessor* accessors;
    uintsize accessor_count;
    
    GltfJson_BufferView* buffer_views;
    uintsize buffer_view_count;
    
    GltfJson_Buffer* buffers;
    uintsize buffer_count;
} typedef Engine_GltfJson;

//~ Functions
internal inline const uint8*
ReadUint32(const uint8* begin, const uint8* end, uint32* out_value)
{
    if (begin + sizeof(uint32) > end)
        return NULL;
    
    memcpy(out_value, begin, sizeof(uint32));
    return begin + sizeof(uint32);
}

internal void
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
        
        if (0 == String_Compare(field_name, Str("generator")))
        {
            if (field_value.kind == Json_ValueKind_String)
                out->asset.generator = Json_RawStringValue(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("version")))
        {
            if (field_value.kind == Json_ValueKind_String)
                out->asset.version = Json_RawStringValue(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("copyright")))
        {
            if (field_value.kind == Json_ValueKind_String)
                out->asset.version = Json_RawStringValue(&field_value);
        }
    }
}

internal bool32
ParseGltfDataNode(const Json_Value* value, GltfJson_Node* out)
{
    if (value->kind != Json_ValueKind_Object)
        return false;
    
    out->name = StrNull;
    out->mesh = -1;
    
    bool32 matrix = false;
    vec3 translation = { 0.0f, 0.0f, 0.0f };
    versor rotation = GLM_QUAT_IDENTITY_INIT;
    vec3 scale = { 1.0f, 1.0f, 1.0f };
    
    for (Json_Field field = { value }; Json_NextField(&field); )
    {
        String field_name = Json_RawFieldName(&field);
        Json_Value field_value;
        Json_FieldValue(&field, &field_value);
        
        if (0 == String_Compare(field_name, Str("name")))
        {
            if (field_value.kind == Json_ValueKind_String)
                out->name = Json_RawStringValue(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("mesh")))
        {
            if (field_value.kind == Json_ValueKind_Number)
                out->mesh = (int32)Json_NumberValueI64(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("translation")))
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
        else if (0 == String_Compare(field_name, Str("rotation")))
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
        else if (0 == String_Compare(field_name, Str("scale")))
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

internal bool32
ParseGltfDataNodes(const Json_Value* value, Engine_GltfJson* out)
{
    if (value->kind != Json_ValueKind_Array)
        return false;
    
    uintsize length = Json_ArrayLength(value);
    if (length == 0)
        return false;
    
    out->nodes = Engine_PushMemory(sizeof(GltfJson_Node) * length);
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

internal bool32
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
        
        if (0 == String_Compare(field_name, Str("name")))
        {
            if (field_value.kind == Json_ValueKind_String)
                out->name = Json_RawStringValue(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("nodes")))
        {
            if (field_value.kind != Json_ValueKind_Array)
                return false;
            
            uintsize length = Json_ArrayLength(&field_value);
            out->nodes = Engine_PushMemory(sizeof(int32) * length);
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

internal bool32
ParseGltfDataScenes(const Json_Value* value, Engine_GltfJson* out)
{
    if (value->kind != Json_ValueKind_Array)
        return false;
    
    uintsize length = Json_ArrayLength(value);
    if (length == 0)
        return false;
    
    out->scenes = Engine_PushMemory(sizeof(GltfJson_Scene) * length);
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

internal bool32
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
        
        if (0 == String_Compare(field_name, Str("indices")))
        {
            if (field_value.kind == Json_ValueKind_Number)
                out->indices = (int32)Json_NumberValueI64(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("mode")))
        {
            if (field_value.kind == Json_ValueKind_Number)
                out->mode = (int32)Json_NumberValueI64(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("attributes")))
        {
            if (field_value.kind != Json_ValueKind_Object)
                return false;
            
            for (Json_Field attrib_field = { &field_value }; Json_NextField(&attrib_field); )
            {
                String attrib_field_name = Json_RawFieldName(&attrib_field);
                Json_Value attrib_field_value;
                Json_FieldValue(&attrib_field, &attrib_field_value);
                
                if (0 == String_Compare(attrib_field_name, Str("POSITION")))
                {
                    if (attrib_field_value.kind == Json_ValueKind_Number)
                        out->attributes.position = (int32)Json_NumberValueI64(&attrib_field_value);
                }
                else if (0 == String_Compare(attrib_field_name, Str("NORMAL")))
                {
                    if (attrib_field_value.kind == Json_ValueKind_Number)
                        out->attributes.normal = (int32)Json_NumberValueI64(&attrib_field_value);
                }
                else if (0 == String_Compare(attrib_field_name, Str("TEXCOORD_0")))
                {
                    if (attrib_field_value.kind == Json_ValueKind_Number)
                        out->attributes.texcoord_0 = (int32)Json_NumberValueI64(&attrib_field_value);
                }
            }
        }
    }
    
    return true;
}

internal bool32
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
        
        if (0 == String_Compare(field_name, Str("name")))
        {
            if (field_value.kind == Json_ValueKind_String)
                out->name = Json_RawStringValue(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("primitives")))
        {
            if (field_value.kind != Json_ValueKind_Array)
                return false;
            
            uintsize length = Json_ArrayLength(&field_value);
            out->primitives = Engine_PushMemory(sizeof(GltfJson_Primitive) * length);
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

internal bool32
ParseGltfDataMeshes(const Json_Value* value, Engine_GltfJson* out)
{
    if (value->kind != Json_ValueKind_Array)
        return false;
    
    uintsize length = Json_ArrayLength(value);
    if (length == 0)
        return false;
    
    out->meshes = Engine_PushMemory(sizeof(GltfJson_Mesh) * length);
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

internal bool32
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
        
        if (0 == String_Compare(field_name, Str("type")))
        {
            if (field_value.kind == Json_ValueKind_String)
                out->type = Json_RawStringValue(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("componentType")))
        {
            if (field_value.kind == Json_ValueKind_Number)
                out->component_type = (int32)Json_NumberValueI64(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("count")))
        {
            if (field_value.kind == Json_ValueKind_Number)
                out->count = (int32)Json_NumberValueI64(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("bufferView")))
        {
            if (field_value.kind == Json_ValueKind_Number)
                out->buffer_view = (int32)Json_NumberValueI64(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("max")))
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
        else if (0 == String_Compare(field_name, Str("min")))
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

internal bool32
ParseGltfDataAccessors(const Json_Value* value, Engine_GltfJson* out)
{
    if (value->kind != Json_ValueKind_Array)
        return false;
    
    uintsize length = Json_ArrayLength(value);
    if (length == 0)
        return false;
    
    out->accessors = Engine_PushMemory(sizeof(GltfJson_Accessor) * length);
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

internal bool32
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
        
        if (0 == String_Compare(field_name, Str("buffer")))
        {
            if (field_value.kind == Json_ValueKind_Number)
                out->buffer = (int32)Json_NumberValueI64(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("byteOffset")))
        {
            if (field_value.kind == Json_ValueKind_Number)
                out->byte_offset = (int32)Json_NumberValueI64(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("byteLength")))
        {
            if (field_value.kind == Json_ValueKind_Number)
                out->byte_length = (int32)Json_NumberValueI64(&field_value);
        }
    }
    
    return true;
}

internal bool32
ParseGltfDataBufferViews(const Json_Value* value, Engine_GltfJson* out)
{
    if (value->kind != Json_ValueKind_Array)
        return false;
    
    uintsize length = Json_ArrayLength(value);
    if (length == 0)
        return false;
    
    out->buffer_views = Engine_PushMemory(sizeof(GltfJson_BufferView) * length);
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

internal bool32
ParseGltfData(const uint8* json_begin, const uint8* json_end,
              const uint8* binary_begin, const uint8* binary_end, Engine_GltfJson* out)
{
    Json_Value json;
    Json_InitFromBufferRange(json_begin, json_end, &json);
    
    for (Json_Field field = { &json }; Json_NextField(&field); )
    {
        String field_name = Json_RawFieldName(&field);
        Json_Value field_value;
        Json_FieldValue(&field, &field_value);
        
        if (0 == String_Compare(field_name, Str("asset")))
        {
            ParseGltfDataAsset(&field_value, out);
        }
        else if (0 == String_Compare(field_name, Str("scene")))
        {
            if (field_value.kind != Json_ValueKind_Number)
                out->scene = -1;
            else
                out->scene = (int32)Json_NumberValueI64(&field_value);
        }
        else if (0 == String_Compare(field_name, Str("scenes")))
        {
            if (!ParseGltfDataScenes(&field_value, out))
                return false;
        }
        else if (0 == String_Compare(field_name, Str("nodes")))
        {
            if (!ParseGltfDataNodes(&field_value, out))
                return false;
        }
        else if (0 == String_Compare(field_name, Str("meshes")))
        {
            if (!ParseGltfDataMeshes(&field_value, out))
                return false;
        }
        else if (0 == String_Compare(field_name, Str("accessors")))
        {
            if (!ParseGltfDataAccessors(&field_value, out))
                return false;
        }
        else if (0 == String_Compare(field_name, Str("bufferViews")))
        {
            if (!ParseGltfDataBufferViews(&field_value, out))
                return false;
        }
        else if (0 == String_Compare(field_name, Str("buffers")))
        {
            if (field_value.kind != Json_ValueKind_Array)
                return false;
            
            uintsize length = Json_ArrayLength(&field_value);
            if (length == 0)
                return false;
            
            out->buffers = Engine_PushMemory(sizeof(GltfJson_Buffer) * length);
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
internal bool32
Engine_ParseGltf(const uint8* data, uintsize size, Engine_GltfJson* out)
{
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
    
    bool32 result = ParseGltfData(json_begin, json_end, binary_begin, binary_end, out);
    
    return result;
}
