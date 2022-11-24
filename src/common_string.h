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

static int32
String_Decode(String str, int32* index)
{
	const uint8* head = str.data + *index;
	const uint8* const end = str.data + str.size;
	
	if (head >= end)
		return 0;
	
	uint8 byte = *head++;
	if (!byte || byte == 0xff)
		return 0;
	
	int32 size =  Mem_BitClz8(~byte);
	if (Unlikely(size == 1 || size > 4 || head + size >= end))
		return 0;
	
	uint32 result = 0;
	if (size == 0)
		result = byte;
	else
	{
		result |= (byte << size & 0xff) >> size;
		
		switch (size)
		{
			case 4: result = (result << 6) | (*head++ & 0x3f);
			case 3: result = (result << 6) | (*head++ & 0x3f);
			case 2: result = (result << 6) | (*head++ & 0x3f);
		}
	}
	
	*index = (int32)(head - str.data);
	return result;
}

// TODO(ljre): make this better (?)
static uintsize
String_DecodedLength(String str)
{
	uintsize len = 0;
	
	int32 it = 0;
	while (String_Decode(str, &it))
		++len;
	
	return len;
}

static inline int32
String_Compare(String a, String b)
{
	int32 result = Mem_Compare(a.data, b.data, Min(a.size, b.size));
	
	if (result == 0 && a.size != b.size)
		return (a.size > b.size) ? 1 : -1;
	
	return result;
}

static inline bool
String_Equals(String a, String b)
{
	if (a.size != b.size)
		return false;
	
	return Mem_Compare(a.data, b.data, a.size) == 0;
}

static inline bool
String_EndsWith(String check, String s)
{
	if (s.size > check.size)
		return false;
	
	String substr = {
		.size = s.size,
		.data = check.data + (check.size - s.size),
	};
	
	return Mem_Compare(substr.data, s.data, substr.size) == 0;
}

#endif // COMMON_STRING_H
