#ifndef COMMON_STRING_H
#define COMMON_STRING_H

struct String
{
	uintsize size;
	const uint8* data;
}
typedef String;

#define Str(x) (String) StrInit(x)
#define StrInit(x) { sizeof(x) - 1, (const uint8*)(x) }
#define StrFmt(x) (x).size, (char*)(x).data
#define StrMake(size,data) (String) { size, data }
#define StrMacro_(x) #x
#define StrMacro(x) StrMacro_(x)
#define StrNull (String) { 0 }

internal int32
String_Decode(const String str, int32* index)
{
	int32 result = 0;
	const uint8* p = str.data + *index;
	if (p >= str.data + str.size || !*p)
		return 0;
	
	uint8 byte = (uint8)*p++;
	
	if (byte & 0b10000000)
	{
		int32 size = 1;
		int32 byte_copy = byte << 1;
		while (byte_copy & 0b10000000)
		{
			++size;
			byte_copy <<= 1;
		}
		
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
String_DecodedLength(const String str)
{
	uintsize len = 0;
	
	int32 it = 0;
	while (String_Decode(str, &it))
		++len;
	
	return len;
}

internal inline int32
String_Compare(const String a, const String b)
{
	if (a.size != b.size)
		return (int32)(a.size - b.size);
	
	return MemCmp(a.data, b.data, a.size);
}

internal inline bool32
String_EndsWith(const String check, const String s)
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
