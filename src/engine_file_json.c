//   JSON Parser
//

#if 0
EXAMPLE USAGE
{
    Json_Value file;
    Json_InitFromBuffer(data, size, &file);
    
    for (Json_Field field = { &file }; Json_NextField(&field); )
    {
        String name = Json_RawFieldName(&field);
        
        if (String_Compare(name, Str("to_print")))
        {
            Json_Value value;
            Json_FieldValue(&field, &value);
            
            String value = Json_RawStringValue(&value);
            
            Print(value);
        }
    }
}
#endif

//~ Types
enum Json_ValueKind
{
    Json_ValueKind_Invalid = 0,
    
    Json_ValueKind_Object,
    Json_ValueKind_Array,
    Json_ValueKind_Number,
    Json_ValueKind_String,
    Json_ValueKind_Bool,
} typedef Json_ValueKind;

struct Json_Value
{
    const uint8* begin;
    const uint8* end;
    
    Json_ValueKind kind;
} typedef Json_Value;

struct Json_Field
{
    const Json_Value* object;
    
    const uint8* begin;
    const uint8* end;
    
    const uint8* name_end;
    const uint8* value_begin;
} typedef Json_Field;

struct Json_ArrayIndex
{
    const Json_Value* object;
    
    const uint8* begin;
    const uint8* end;
} typedef Json_ArrayIndex;

//~ Functions
internal inline bool32
IsWhiteSpace(uint8 c)
{
    return (c == ' ') | (c == '\t') | (c == '\n') | (c == '\r');
}

internal inline const uint8*
IgnoreWhiteSpacesLeft(const uint8* begin, const uint8* end)
{
    while (begin < end && IsWhiteSpace(*begin))
        ++begin;
    
    return begin;
}

internal inline const uint8*
IgnoreWhiteSpacesRight(const uint8* begin, const uint8* end)
{
    while (begin + 1 < end && IsWhiteSpace(end[-1]))
        --end;
    
    return end;
}

internal const uint8*
FindEndOfValue(const uint8* begin, const uint8* end)
{
    const uint8* it = begin;
    
    switch (*it++)
    {
        case '{':
        {
            int32 nested = 1;
            
            while (nested > 0 && it < end)
            {
                if (it[0] == '{') ++nested;
                else if (it[0] == '}') --nested;
                
                ++it;
            }
            
            if (nested > 0)
                return NULL;
        } break;
        
        case '[': // NOTE(ljre): yeah i know it's duplicated code
        {
            int32 nested = 1;
            
            while (nested > 0 && it < end)
            {
                if (it[0] == '[') ++nested;
                else if (it[0] == ']') --nested;
                
                ++it;
            }
            
            if (nested > 0)
                return NULL;
        } break;
        
        case '"':
        {
            while (it < end && it[0] != '"' && it[-1] != '\\')
                ++it;
            
            if (++it >= end)
                return NULL;
        } break;
        
        case '-': case '0':
        case '1': case '2': case '3': case'4':
        case '5': case '6': case '7': case'8':
        case '9':
        {
            while (it < end && ((it[0] >= '0' && it[0] <= '9') || it[0] == '.'))
                ++it;
            
            if (it >= end)
                return NULL;
        } break;
        
        default:
        {
            --it;
            while (it < end && (*it >= 'a' && *it <= 'z'))
                ++it;
        } break;
    }
    
    return it;
}

//~ Internal API
internal String
Json_RawFieldName(const Json_Field* field)
{
    String result;
    
    result.data = (const char*)field->begin + 1;
    result.len = (uintsize)((const char*)field->name_end - result.data);
    
    return result;
}

internal float64
Json_NumberValueF64(const Json_Value* value)
{
    Assert(value);
    Assert(value->begin);
    Assert(value->kind == Json_ValueKind_Number);
    
    uintsize len = (uintsize)(value->end - value->begin);
    
    Assert(len < 128);
    
    char buf[128];
    memcpy(buf, value->begin, len);
    buf[len] = 0;
    
    return strtod(buf, NULL);
}

internal int64
Json_NumberValueI64(const Json_Value* value)
{
    Assert(value);
    Assert(value->begin);
    Assert(value->kind == Json_ValueKind_Number);
    
    uintsize len = (uintsize)(value->end - value->begin);
    
    Assert(len < 128);
    
    char buf[128];
    memcpy(buf, value->begin, len);
    buf[len] = 0;
    
    return strtoll(buf, NULL, 10);
}

internal bool32
Json_BoolValue(const Json_Value* value)
{
    Assert(value);
    Assert(value->begin);
    Assert(value->kind == Json_ValueKind_Bool);
    
    return strncmp((const char*)value->begin, "true", (uintsize)(value->end - value->begin)) == 0;
}

internal String
Json_RawStringValue(const Json_Value* value)
{
    Assert(value);
    Assert(value->begin);
    Assert(value->kind == Json_ValueKind_String);
    
    String result;
    
    result.data = (const char*)value->begin + 1;
    result.len = (uintsize)value->end - (uintsize)result.data - 1;
    
    return result;
}

internal bool32
Json_NextIndex(Json_ArrayIndex* index)
{
    if (!index)
        return false;
    
    const Json_Value* value = index->object;
    Assert(value->kind == Json_ValueKind_Array);
    Assert(value->begin < value->end);
    
    if (!index->begin)
    {
        index->begin = IgnoreWhiteSpacesLeft(value->begin + 1, value->end);
        index->end = IgnoreWhiteSpacesRight(index->begin, value->end);
        
        if (index->begin >= index->end)
            return false;
    }
    else
    {
        index->end = IgnoreWhiteSpacesLeft(index->end, value->end);
        
        if (index->end >= value->end || index->end[0] != ',')
            return false;
        
        ++index->end; // ignore ,
        index->end = IgnoreWhiteSpacesLeft(index->end, value->end);
        
        if (index->end >= value->end || index->end[0] == ']')
            return false;
        
        index->begin = index->end;
        index->end = value->end;
    }
    
    index->end = FindEndOfValue(index->begin, index->end);
    if (!index->end)
        return false;
    
    return true;
}

internal void
Json_IndexValue(const Json_ArrayIndex* index, Json_Value* value)
{
    Assert(index);
    Assert(value);
    Assert(index->begin < index->end);
    
    value->begin = index->begin;
    value->end = index->end;
    
    switch (*value->begin)
    {
        case '{': value->kind = Json_ValueKind_Object; break;
        case '[': value->kind = Json_ValueKind_Array; break;
        case '"': value->kind = Json_ValueKind_String; break;
        
        case '-': case '0':
        case '1': case '2': case '3': case'4':
        case '5': case '6': case '7': case'8':
        case '9': value->kind = Json_ValueKind_Number; break;
        
        default:
        {
            uintsize len = (uintsize)(value->end - value->begin);
            
            if (strncmp((const char*)value->begin, "true", len) == 0 ||
                strncmp((const char*)value->begin, "false", len) == 0)
            {
                value->kind = Json_ValueKind_Bool;
            }
            else
            {
                value->kind = Json_ValueKind_Invalid;
            }
        } break;
    }
}

internal uintsize
Json_ArrayLength(const Json_Value* value)
{
    Assert(value);
    Assert(value->kind == Json_ValueKind_Array);
    
    uintsize len = 0;
    for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); )
        ++len;
    
    return len;
}

internal bool32
Json_NextField(Json_Field* field)
{
    if (!field)
        return false;
    
    const Json_Value* value = field->object;
    Assert(value->kind == Json_ValueKind_Object);
    Assert(value->begin < value->end);
    
    if (!field->begin)
    {
        field->begin = IgnoreWhiteSpacesLeft(value->begin + 1, value->end);
        field->end = IgnoreWhiteSpacesRight(field->begin, value->end);
        
        if (field->begin + 2 >= field->end)
            return false;
    }
    else
    {
        field->end = IgnoreWhiteSpacesLeft(field->end, value->end);
        
        if (field->end >= value->end || field->end[0] != ',')
            return false;
        
        ++field->end; // ignore ,
        field->end = IgnoreWhiteSpacesLeft(field->end, value->end);
        
        if (field->end >= value->end || field->end[0] == '}')
            return false;
        
        field->begin = field->end;
        field->end = value->end;
    }
    
    // NOTE(ljre): Parse name
    const uint8* name_begin = field->begin;
    if (*name_begin != '"')
        return false;
    
    const uint8* it = name_begin + 1;
    while (it[0] != '"' && it[-1] != '\\')
    {
        ++it;
        if (it >= field->end)
            return false;
    }
    
    field->name_end = it;
    
    // NOTE(ljre): ':'
    it = IgnoreWhiteSpacesLeft(it + 1, field->end);
    if (it >= field->end || it[0] != ':')
        return false;
    
    it = IgnoreWhiteSpacesLeft(it + 1, field->end);
    if (it + 1 >= field->end)
        return false;
    
    // NOTE(ljre): Value
    field->value_begin = it;
    
    it = FindEndOfValue(it, field->end);
    if (!it)
        return false;
    
    field->end = it;
    
    return true;
}

internal void
Json_FieldValue(const Json_Field* field, Json_Value* value)
{
    Assert(field);
    Assert(value);
    Assert(field->begin < field->end);
    
    value->begin = field->value_begin;
    value->end = field->end;
    
    switch (*value->begin)
    {
        case '{': value->kind = Json_ValueKind_Object; break;
        case '[': value->kind = Json_ValueKind_Array; break;
        case '"': value->kind = Json_ValueKind_String; break;
        
        case '-': case '0':
        case '1': case '2': case '3': case'4':
        case '5': case '6': case '7': case'8':
        case '9': value->kind = Json_ValueKind_Number; break;
        
        default:
        {
            uintsize len = (uintsize)(value->end - value->begin);
            
            if (strncmp((const char*)value->begin, "true", len) == 0 ||
                strncmp((const char*)value->begin, "false", len) == 0)
            {
                value->kind = Json_ValueKind_Bool;
            }
            else
            {
                value->kind = Json_ValueKind_Invalid;
            }
        } break;
    }
}

internal bool32
Json_FindField(const Json_Value* object, String name, Json_Field* out)
{
    Assert(object);
    Assert(object->kind == Json_ValueKind_Object);
    
    Json_Field field = { object };
    while (Json_NextField(&field))
    {
        if (String_Compare(Json_RawFieldName(&field), name) == 0)
        {
            *out = field;
            return true;
        }
    }
    
    return false;
}

internal bool32
Json_FindIndex(const Json_Value* array, int32 i, Json_ArrayIndex* out)
{
    Assert(array);
    Assert(array->kind == Json_ValueKind_Array);
    
    Json_ArrayIndex index = { array };
    int32 current_i = 0;
    while (Json_NextIndex(&index))
    {
        if (current_i == i)
        {
            *out = index;
            return true;
        }
    }
    
    return false;
}

internal bool32
Json_FindFieldValue(const Json_Value* object, String name, Json_Value* out)
{
    Json_Field field;
    if (!Json_FindField(object, name, &field))
        return false;
    
    Json_FieldValue(&field, out);
    return true;
}

internal bool32
Json_FindIndexValue(const Json_Value* array, int32 i, Json_Value* out)
{
    Json_ArrayIndex index;
    if (!Json_FindIndex(array, i, &index))
        return false;
    
    Json_IndexValue(&index, out);
    return true;
}

internal void
Json_InitFromBufferRange(const uint8* begin, const uint8* end, Json_Value* out_state)
{
    if (begin + 1 < end)
    {
        out_state->begin = begin;
        out_state->end = end;
        
        out_state->begin = IgnoreWhiteSpacesLeft(out_state->begin, out_state->end);
        if (out_state->begin >= out_state->end)
        {
            out_state->kind = Json_ValueKind_Invalid;
        }
        else
        {
            switch (*out_state->begin)
            {
                case '{': out_state->kind = Json_ValueKind_Object; break;
                case '[': out_state->kind = Json_ValueKind_Array; break;
                case '"': out_state->kind = Json_ValueKind_String; break;
                
                case '-': case '0':
                case '1': case '2': case '3': case'4':
                case '5': case '6': case '7': case'8':
                case '9': out_state->kind = Json_ValueKind_Number; break;
                
                default:
                {
                    uintsize len = (uintsize)(out_state->end - out_state->begin);
                    
                    if (strncmp((const char*)out_state->begin, "true", len) == 0 ||
                        strncmp((const char*)out_state->begin, "false", len) == 0)
                    {
                        out_state->kind = Json_ValueKind_Bool;
                    }
                    else
                    {
                        out_state->kind = Json_ValueKind_Invalid;
                    }
                } break;
            }
        }
    }
    else
    {
        out_state->kind = Json_ValueKind_Invalid;
    }
}

internal inline void
Json_InitFromBuffer(const uint8* data, uintsize size, Json_Value* out_state)
{
    Json_InitFromBufferRange(data, data + size, out_state);
}
