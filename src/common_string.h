#ifndef COMMON_STRING_H
#define COMMON_STRING_H

struct String
{
	uintsize size;
	const uint8* data;
}
typedef String;

#ifndef __cplusplus
#   define Str(x) (String) StrInit(x)
#   define StrNull (String) { 0 }
#   define StrMake(size,data) (String) { (uintsize)(size), (const uint8*)(data) }
#else
#   define Str(x) String StrInit(x)
#   define StrNull String {}
#   define StrMake(size, data) String { (uintsize)(size), (const uint8*)(data) }
#endif

#define StrInit(x) { sizeof(x) - 1, (const uint8*)(x) }
#define StrFmt(x) (x).size, (char*)(x).data
#define StrMacro_(x) #x
#define StrMacro(x) StrMacro_(x)

internal int32
String_Decode(String str, int32* index)
{
	int32 result = 0;
	const uint8* p = str.data + *index;
	if (p >= str.data + str.size || !*p)
		return 0;
	
	uint8 byte = (uint8)*p++;
	
	if (byte & 0b10000000)
	{
		int32 size = BitClz(~byte);
		
		if (size == 1)
		{
			// NOTE(ljre): Continuation byte. Something is wrong.
			result = 0;
		}
		else
		{
			Assert(size < 5);
			
			result |= byte & ((1 << (8-size)) - 1);
			while (size > 1)
			{
				result <<= 6;
				byte = (uint8)*p++;
				result |= byte & 0b00111111;
				
				--size;
			}
		}
	}
	else
	{
		result |= byte;
	}
	
	// Return
	*index = (int32)(p - str.data);
	return result;
}

// TODO(ljre): make this better (?)
internal uintsize
String_DecodedLength(String str)
{
	uintsize len = 0;
	
	int32 it = 0;
	while (String_Decode(str, &it))
		++len;
	
	return len;
}

internal inline int32
String_Compare(String a, String b)
{
	int32 result = MemCmp(a.data, b.data, Min(a.size, b.size));
	
	if (result == 0 && a.size != b.size)
		return (a.size > b.size) ? 1 : -1;
	
	return result;
}

internal inline bool
String_Equals(String a, String b)
{
	if (a.size != b.size)
		return false;
	
	return MemCmp(a.data, b.data, a.size) == 0;
}

internal inline bool
String_EndsWith(String check, String s)
{
	if (s.size > check.size)
		return false;
	
	String substr = {
		.size = s.size,
		.data = check.data + (check.size - s.size),
	};
	
	return MemCmp(substr.data, s.data, substr.size) == 0;
}

#endif // COMMON_STRING_H
