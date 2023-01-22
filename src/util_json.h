#ifndef UTIL_JSON_H
#define UTIL_JSON_H

//   JSON Parser
//

#if 0
EXAMPLE USAGE
{
	// NOTE(ljre): 'file' is expected to be an object
	UJson_Value file;
	UJson_InitFromBuffer(data, size, &file);
	
	// NOTE(ljre): iterate through every field of the object
	for (UJson_Field field = { &file }; UJson_NextField(&field); )
	{
		// NOTE(ljre): "raw name" = without processing escape sequences, etc.
		String name = UJson_RawFieldName(&field);
		
		if (String_Equals(name, Str("to_print")))
		{
			UJson_Value value;
			UJson_FieldValue(&field, &value);
			
			String value = UJson_RawStringValue(&value);
			
			Print(value);
		}
	}
}
#endif

//~ Types
enum UJson_ValueKind
{
	UJson_ValueKind_Invalid = 0,
	
	UJson_ValueKind_Object,
	UJson_ValueKind_Array,
	UJson_ValueKind_Number,
	UJson_ValueKind_String,
	UJson_ValueKind_Bool,
	UJson_ValueKind_Null,
}
typedef UJson_ValueKind;

struct UJson_Value
{
	const uint8* begin;
	const uint8* end;
	
	UJson_ValueKind kind;
}
typedef UJson_Value;

struct UJson_Field
{
	const UJson_Value* object;
	
	const uint8* begin;
	const uint8* end;
	
	const uint8* name_end;
	const uint8* value_begin;
}
typedef UJson_Field;

struct UJson_ArrayIndex
{
	const UJson_Value* object;
	
	const uint8* begin;
	const uint8* end;
}
typedef UJson_ArrayIndex;

//~ Functions
static inline bool
UJson_IsWhiteSpace_(uint8 c)
{
	return (c == ' ') | (c == '\t') | (c == '\n') | (c == '\r');
}

static inline const uint8*
UJson_IgnoreWhiteSpacesLeft_(const uint8* begin, const uint8* end)
{
	while (begin < end && UJson_IsWhiteSpace_(*begin))
		++begin;
	
	return begin;
}

static inline const uint8*
UJson_IgnoreWhiteSpacesRight_(const uint8* begin, const uint8* end)
{
	while (begin + 1 < end && UJson_IsWhiteSpace_(end[-1]))
		--end;
	
	return end;
}

static const uint8*
UJson_FindEndOfValue_(const uint8* begin, const uint8* end)
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

static UJson_ValueKind
UJson_CalcValueKind_(const UJson_Value* value)
{
	switch (*value->begin)
	{
		case '{': return UJson_ValueKind_Object; break;
		case '[': return UJson_ValueKind_Array; break;
		case '"': return UJson_ValueKind_String; break;
		
		case '-': case '0':
		case '1': case '2': case '3': case'4':
		case '5': case '6': case '7': case'8':
		case '9': return UJson_ValueKind_Number; break;
		
		default:
		{
			uintsize len = (uintsize)(value->end - value->begin);
			String str = { .size = len, .data = value->begin };
			
			if (String_Equals(str, Str("true")) || String_Equals(str, Str("false")))
				return UJson_ValueKind_Bool;
			else if (String_Equals(str, Str("null")))
				return UJson_ValueKind_Null;
			else
				return UJson_ValueKind_Invalid;
		} break;
	}
	
	return UJson_ValueKind_Invalid;
}

//~ Internal API
static String
UJson_RawFieldName(const UJson_Field* field)
{
	return StrMake(field->name_end - (field->begin + 1), field->begin + 1);
}

static float64
UJson_NumberValueF64(const UJson_Value* value)
{
	Assert(value);
	Assert(value->begin);
	Assert(value->kind == UJson_ValueKind_Number);
	
	uintsize len = (uintsize)(value->end - value->begin);
	
	Assert(len < 127);
	
	char buf[128];
	Mem_Copy(buf, value->begin, len);
	buf[len] = 0;
	
	return strtod(buf, NULL);
}

static int64
UJson_NumberValueI64(const UJson_Value* value)
{
	Assert(value);
	Assert(value->begin);
	Assert(value->kind == UJson_ValueKind_Number);
	
	uintsize len = (uintsize)(value->end - value->begin);
	
	Assert(len < 127);
	
	char buf[128];
	Mem_Copy(buf, value->begin, len);
	buf[len] = 0;
	
	return strtoll(buf, NULL, 10);
}

static bool
UJson_BoolValue(const UJson_Value* value)
{
	Assert(value);
	Assert(value->begin);
	Assert(value->kind == UJson_ValueKind_Bool);
	Assert(value->end - value->begin >= 4);
	
	return (value->end - value->begin) == 4;
}

static String
UJson_RawStringValue(const UJson_Value* value)
{
	Assert(value);
	Assert(value->begin);
	Assert(value->kind == UJson_ValueKind_String);
	
	String result;
	
	result.data = (uint8*)value->begin + 1;
	result.size = (uintsize)value->end - (uintsize)result.data - 1;
	
	return result;
}

static bool
UJson_NextIndex(UJson_ArrayIndex* index)
{
	if (!index)
		return false;
	
	const UJson_Value* value = index->object;
	Assert(value->kind == UJson_ValueKind_Array);
	Assert(value->begin < value->end);
	
	if (!index->begin)
	{
		index->begin = UJson_IgnoreWhiteSpacesLeft_(value->begin + 1, value->end);
		index->end = UJson_IgnoreWhiteSpacesRight_(index->begin, value->end);
		
		if (index->begin >= index->end)
			return false;
	}
	else
	{
		index->end = UJson_IgnoreWhiteSpacesLeft_(index->end, value->end);
		
		if (index->end >= value->end || index->end[0] != ',')
			return false;
		
		++index->end; // ignore ,
		index->end = UJson_IgnoreWhiteSpacesLeft_(index->end, value->end);
		
		if (index->end >= value->end || index->end[0] == ']')
			return false;
		
		index->begin = index->end;
		index->end = value->end;
	}
	
	index->end = UJson_FindEndOfValue_(index->begin, index->end);
	if (!index->end)
		return false;
	
	return true;
}

static void
UJson_IndexValue(const UJson_ArrayIndex* index, UJson_Value* value)
{
	Assert(index);
	Assert(value);
	Assert(index->begin < index->end);
	
	value->begin = index->begin;
	value->end = index->end;
	
	value->kind = UJson_CalcValueKind_(value);
}

static uintsize
UJson_ArrayLength(const UJson_Value* value)
{
	Assert(value);
	Assert(value->kind == UJson_ValueKind_Array);
	
	uintsize len = 0;
	for (UJson_ArrayIndex index = { value }; UJson_NextIndex(&index); )
		++len;
	
	return len;
}

static bool
UJson_NextField(UJson_Field* field)
{
	if (!field)
		return false;
	
	const UJson_Value* value = field->object;
	Assert(value->kind == UJson_ValueKind_Object);
	Assert(value->begin < value->end);
	
	if (!field->begin)
	{
		field->begin = UJson_IgnoreWhiteSpacesLeft_(value->begin + 1, value->end);
		field->end = UJson_IgnoreWhiteSpacesRight_(field->begin, value->end);
		
		if (field->begin + 2 >= field->end)
			return false;
	}
	else
	{
		field->end = UJson_IgnoreWhiteSpacesLeft_(field->end, value->end);
		
		if (field->end >= value->end || field->end[0] != ',')
			return false;
		
		++field->end; // ignore ,
		field->end = UJson_IgnoreWhiteSpacesLeft_(field->end, value->end);
		
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
	it = UJson_IgnoreWhiteSpacesLeft_(it + 1, field->end);
	if (it >= field->end || it[0] != ':')
		return false;
	
	it = UJson_IgnoreWhiteSpacesLeft_(it + 1, field->end);
	if (it + 1 >= field->end)
		return false;
	
	// NOTE(ljre): Value
	field->value_begin = it;
	
	it = UJson_FindEndOfValue_(it, field->end);
	if (!it)
		return false;
	
	field->end = it;
	
	return true;
}

static void
UJson_FieldValue(const UJson_Field* field, UJson_Value* value)
{
	Assert(field);
	Assert(value);
	Assert(field->begin < field->end);
	
	value->begin = field->value_begin;
	value->end = field->end;
	
	value->kind = UJson_CalcValueKind_(value);
}

static bool
UJson_FindField(const UJson_Value* object, String name, UJson_Field* out)
{
	Assert(object);
	Assert(object->kind == UJson_ValueKind_Object);
	
	UJson_Field field = { object };
	while (UJson_NextField(&field))
	{
		if (String_Equals(UJson_RawFieldName(&field), name))
		{
			*out = field;
			return true;
		}
	}
	
	return false;
}

static bool
UJson_FindIndex(const UJson_Value* array, int32 i, UJson_ArrayIndex* out)
{
	Assert(array);
	Assert(array->kind == UJson_ValueKind_Array);
	
	UJson_ArrayIndex index = { array };
	int32 current_i = 0;
	while (UJson_NextIndex(&index))
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
UJson_ReadWholeArray(const UJson_Value* array, UJson_Value* out_entries, uintsize max_entry_count)
{
	Assert(array);
	Assert(array->kind == UJson_ValueKind_Array);
	
	uintsize i = 0;
	for (UJson_ArrayIndex index = { array }; i < max_entry_count && UJson_NextIndex(&index); ++i)
		UJson_IndexValue(&index, &out_entries[i]);
	
	return i;
}

static bool
UJson_FindFieldValue(const UJson_Value* object, String name, UJson_Value* out)
{
	UJson_Field field;
	if (!UJson_FindField(object, name, &field))
		return false;
	
	UJson_FieldValue(&field, out);
	return true;
}

static bool
UJson_FindIndexValue(const UJson_Value* array, int32 i, UJson_Value* out)
{
	UJson_ArrayIndex index;
	if (!UJson_FindIndex(array, i, &index))
		return false;
	
	UJson_IndexValue(&index, out);
	return true;
}

static void
UJson_InitFromBufferRange(const uint8* begin, const uint8* end, UJson_Value* out_state)
{
	if (begin + 1 < end)
	{
		out_state->begin = begin;
		out_state->end = end;
		
		out_state->begin = UJson_IgnoreWhiteSpacesLeft_(out_state->begin, out_state->end);
		if (out_state->begin >= out_state->end)
			out_state->kind = UJson_ValueKind_Invalid;
		else
			out_state->kind = UJson_CalcValueKind_(out_state);
	}
	else
		out_state->kind = UJson_ValueKind_Invalid;
}

static inline void
UJson_InitFromBuffer(const uint8* data, uintsize size, UJson_Value* out_state)
{ UJson_InitFromBufferRange(data, data + size, out_state); }

#endif //UTIL_JSON_H
