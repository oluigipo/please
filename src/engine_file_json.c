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
            
            if (it >= end)
                return NULL;
        } break;
        
        // case '0': // NOTE(ljre): a number cannot start with 0... really
        case '1': case '2': case '3': case'4':
        case '5': case '6': case '7': case'8':
        case '9':
        {
            while (it < end && ((it[0] >= '0' && it[0] <= '9') || it[0] == '.'))
                ++it;
            
            if (it >= end)
                return NULL;
        } break;
        
        default: return NULL;
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
    Assert(value->kind == Json_ValueKind_Object);
    Assert(value->begin < value->end);
    
    if (!index->begin)
    {
        index->begin = IgnoreWhiteSpacesLeft(value->begin + 1, value->end);
        index->end = IgnoreWhiteSpacesRight(index->begin, value->end);
        
        if (index->begin + 2 >= index->end)
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
    
    index->end = FindEndOfValue(index->end, index->end);
    if (!index->end)
        return false;
    
    return true;
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
    it = IgnoreWhiteSpacesLeft(it, field->end);
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
    
    switch (*field->begin)
    {
        case '{': value->kind = Json_ValueKind_Object; break;
        case '[': value->kind = Json_ValueKind_Array; break;
        case '"': value->kind = Json_ValueKind_String; break;
        
        case '1': case '2': case '3': case'4':
        case '5': case '6': case '7': case'8':
        case '9': value->kind = Json_ValueKind_Number; break;
        
        default: value->kind = Json_ValueKind_Invalid; break;
    }
}

internal void
Json_InitFromBuffer(const uint8* data, uintsize size, Json_Value* out_state)
{
    if (size > 0)
    {
        out_state->begin = data;
        out_state->end = data + size;
        
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
                
                case '1': case '2': case '3': case'4':
                case '5': case '6': case '7': case'8':
                case '9': out_state->kind = Json_ValueKind_Number; break;
                
                default: out_state->kind = Json_ValueKind_Invalid; break;
            }
        }
    }
    else
    {
        out_state->kind = Json_ValueKind_Invalid;
    }
}
