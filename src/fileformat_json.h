#ifndef JSON_H
#define JSON_H

//   JSON Parser
//

#if 0
EXAMPLE USAGE
{
	// NOTE(ljre): 'file' is expected to be an object
	Json_Value file;
	Json_InitFromBuffer(data, size, &file);
	
	// NOTE(ljre): iterate through every field of the object
	for (Json_Field field = { &file }; Json_NextField(&field); )
	{
		// NOTE(ljre): "raw name" = without processing escape sequences, etc.
		String name = Json_RawFieldName(&field);
		
		if (String_Equals(name, Str("to_print")))
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
	Json_ValueKind_Null,
}
typedef Json_ValueKind;

struct Json_Value
{
	const uint8* begin;
	const uint8* end;
	
	Json_ValueKind kind;
}
typedef Json_Value;

struct Json_Field
{
	const Json_Value* object;
	
	const uint8* begin;
	const uint8* end;
	
	const uint8* name_end;
	const uint8* value_begin;
}
typedef Json_Field;

struct Json_ArrayIndex
{
	const Json_Value* object;
	
	const uint8* begin;
	const uint8* end;
}
typedef Json_ArrayIndex;

//~ Functions
static inline bool
Json_IsWhiteSpace_(uint8 c)
{
	return (c == ' ') | (c == '\t') | (c == '\n') | (c == '\r');
}

static inline const uint8*
Json_IgnoreWhiteSpacesLeft_(const uint8* begin, const uint8* end)
{
	while (begin < end && Json_IsWhiteSpace_(*begin))
		++begin;
	
	return begin;
}

static inline const uint8*
Json_IgnoreWhiteSpacesRight_(const uint8* begin, const uint8* end)
{
	while (begin + 1 < end && Json_IsWhiteSpace_(end[-1]))
		--end;
	
	return end;
}

static const uint8*
Json_FindEndOfValue_(const uint8* begin, const uint8* end)
{
	const uint8* it = begin;
	
	switch (*it++)
	{
		case '{':
		{
			int32 nested = 1;
			
			while (nested > 0 && it < end)
			{
				if (it[0] == '{')
					++nested;
				else if (it[0] == '}')
					--nested;
				
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
				if (it[0] == '[')
					++nested;
				else if (it[0] == ']')
					--nested;
				
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

static Json_ValueKind
Json_CalcValueKind_(const Json_Value* value)
{
	switch (*value->begin)
	{
		case '{': return Json_ValueKind_Object; break;
		case '[': return Json_ValueKind_Array; break;
		case '"': return Json_ValueKind_String; break;
		
		case '-': case '0':
		case '1': case '2': case '3': case'4':
		case '5': case '6': case '7': case'8':
		case '9': return Json_ValueKind_Number; break;
		
		default:
		{
			uintsize len = (uintsize)(value->end - value->begin);
			String str = { .size = len, .data = value->begin };
			
			if (String_Equals(str, Str("true")) || String_Equals(str, Str("false")))
				return Json_ValueKind_Bool;
			else if (String_Equals(str, Str("null")))
				return Json_ValueKind_Null;
			else
				return Json_ValueKind_Invalid;
		} break;
	}
	
	return Json_ValueKind_Invalid;
}

//~ Internal API
static String
Json_RawFieldName(const Json_Field* field)
{
	return StrMake(field->name_end - (field->begin + 1), field->begin + 1);
}

static float64
Json_NumberValueF64(const Json_Value* value)
{
	Assert(value);
	Assert(value->begin);
	Assert(value->kind == Json_ValueKind_Number);
	
	uintsize len = (uintsize)(value->end - value->begin);
	
	Assert(len < 127);
	
	char buf[128];
	Mem_Copy(buf, value->begin, len);
	buf[len] = 0;
	
	return strtod(buf, NULL);
}

static int64
Json_NumberValueI64(const Json_Value* value)
{
	Assert(value);
	Assert(value->begin);
	Assert(value->kind == Json_ValueKind_Number);
	
	uintsize len = (uintsize)(value->end - value->begin);
	
	Assert(len < 127);
	
	char buf[128];
	Mem_Copy(buf, value->begin, len);
	buf[len] = 0;
	
	return strtoll(buf, NULL, 10);
}

static bool
Json_BoolValue(const Json_Value* value)
{
	Assert(value);
	Assert(value->begin);
	Assert(value->kind == Json_ValueKind_Bool);
	Assert(value->end - value->begin >= 4);
	
	return (value->end - value->begin) == 4;
}

static String
Json_RawStringValue(const Json_Value* value)
{
	Assert(value);
	Assert(value->begin);
	Assert(value->kind == Json_ValueKind_String);
	
	String result;
	
	result.data = (uint8*)value->begin + 1;
	result.size = (uintsize)value->end - (uintsize)result.data - 1;
	
	return result;
}

static bool
Json_NextIndex(Json_ArrayIndex* index)
{
	if (!index)
		return false;
	
	const Json_Value* value = index->object;
	Assert(value->kind == Json_ValueKind_Array);
	Assert(value->begin < value->end);
	
	if (!index->begin)
	{
		index->begin = Json_IgnoreWhiteSpacesLeft_(value->begin + 1, value->end);
		index->end = Json_IgnoreWhiteSpacesRight_(index->begin, value->end);
		
		if (index->begin >= index->end)
			return false;
	}
	else
	{
		index->end = Json_IgnoreWhiteSpacesLeft_(index->end, value->end);
		
		if (index->end >= value->end || index->end[0] != ',')
			return false;
		
		++index->end; // ignore ,
		index->end = Json_IgnoreWhiteSpacesLeft_(index->end, value->end);
		
		if (index->end >= value->end || index->end[0] == ']')
			return false;
		
		index->begin = index->end;
		index->end = value->end;
	}
	
	index->end = Json_FindEndOfValue_(index->begin, index->end);
	if (!index->end)
		return false;
	
	return true;
}

static void
Json_IndexValue(const Json_ArrayIndex* index, Json_Value* value)
{
	Assert(index);
	Assert(value);
	Assert(index->begin < index->end);
	
	value->begin = index->begin;
	value->end = index->end;
	
	value->kind = Json_CalcValueKind_(value);
}

static uintsize
Json_ArrayLength(const Json_Value* value)
{
	Assert(value);
	Assert(value->kind == Json_ValueKind_Array);
	
	uintsize len = 0;
	for (Json_ArrayIndex index = { value }; Json_NextIndex(&index); )
		++len;
	
	return len;
}

static bool
Json_NextField(Json_Field* field)
{
	if (!field)
		return false;
	
	const Json_Value* value = field->object;
	Assert(value->kind == Json_ValueKind_Object);
	Assert(value->begin < value->end);
	
	if (!field->begin)
	{
		field->begin = Json_IgnoreWhiteSpacesLeft_(value->begin + 1, value->end);
		field->end = Json_IgnoreWhiteSpacesRight_(field->begin, value->end);
		
		if (field->begin + 2 >= field->end)
			return false;
	}
	else
	{
		field->end = Json_IgnoreWhiteSpacesLeft_(field->end, value->end);
		
		if (field->end >= value->end || field->end[0] != ',')
			return false;
		
		++field->end; // ignore ,
		field->end = Json_IgnoreWhiteSpacesLeft_(field->end, value->end);
		
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
	it = Json_IgnoreWhiteSpacesLeft_(it + 1, field->end);
	if (it >= field->end || it[0] != ':')
		return false;
	
	it = Json_IgnoreWhiteSpacesLeft_(it + 1, field->end);
	if (it + 1 >= field->end)
		return false;
	
	// NOTE(ljre): Value
	field->value_begin = it;
	
	it = Json_FindEndOfValue_(it, field->end);
	if (!it)
		return false;
	
	field->end = it;
	
	return true;
}

static void
Json_FieldValue(const Json_Field* field, Json_Value* value)
{
	Assert(field);
	Assert(value);
	Assert(field->begin < field->end);
	
	value->begin = field->value_begin;
	value->end = field->end;
	
	value->kind = Json_CalcValueKind_(value);
}

static bool
Json_FindField(const Json_Value* object, String name, Json_Field* out)
{
	Assert(object);
	Assert(object->kind == Json_ValueKind_Object);
	
	Json_Field field = { object };
	while (Json_NextField(&field))
	{
		if (String_Equals(Json_RawFieldName(&field), name))
		{
			*out = field;
			return true;
		}
	}
	
	return false;
}

static bool
Json_FindIndex(const Json_Value* array, int32 i, Json_ArrayIndex* out)
{
	Assert(array);
	Assert(array->kind == Json_ValueKind_Array);
	
	Json_ArrayIndex index = { array };
	int32 current_i = 0;
	while (Json_NextIndex(&index))
	{
		if (current_i++ == i)
		{
			*out = index;
			return true;
		}
	}
	
	return false;
}

static uintsize
Json_ReadWholeArray(const Json_Value* array, Json_Value* out_entries, uintsize max_entry_count)
{
	Assert(array);
	Assert(array->kind == Json_ValueKind_Array);
	
	uintsize i = 0;
	for (Json_ArrayIndex index = { array }; i < max_entry_count && Json_NextIndex(&index); ++i)
		Json_IndexValue(&index, &out_entries[i]);
	
	return i;
}

static bool
Json_FindFieldValue(const Json_Value* object, String name, Json_Value* out)
{
	Json_Field field;
	if (!Json_FindField(object, name, &field))
		return false;
	
	Json_FieldValue(&field, out);
	return true;
}

static bool
Json_FindIndexValue(const Json_Value* array, int32 i, Json_Value* out)
{
	Json_ArrayIndex index;
	if (!Json_FindIndex(array, i, &index))
		return false;
	
	Json_IndexValue(&index, out);
	return true;
}

static void
Json_InitFromBufferRange(const uint8* begin, const uint8* end, Json_Value* out_state)
{
	if (begin + 1 < end)
	{
		out_state->begin = begin;
		out_state->end = end;
		
		out_state->begin = Json_IgnoreWhiteSpacesLeft_(out_state->begin, out_state->end);
		if (out_state->begin >= out_state->end)
			out_state->kind = Json_ValueKind_Invalid;
		else
			out_state->kind = Json_CalcValueKind_(out_state);
	}
	else
		out_state->kind = Json_ValueKind_Invalid;
}

static inline void
Json_InitFromBuffer(const uint8* data, uintsize size, Json_Value* out_state)
{ Json_InitFromBufferRange(data, data + size, out_state); }

#endif //JSON_H
