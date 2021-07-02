#ifndef INTERNAL_STRING_H
#define INTERNAL_STRING_H

struct String
{
    uintsize len; // NOTE(ljre): length IN BYTES and MAY include the null terminator!!!!!
    const char* data;
} typedef String;

#define Str(x) (String) StrI(x)
#define StrI(x) { sizeof(x), (x) }
#define StrFmt(x) (x).len, (x).data

internal int32
String_Decode(String str, int32* index)
{
    int32 result = 0;
    const char* p = str.data + *index;
    if (p >= str.data + str.len || !*p)
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
String_Length(String str)
{
    uintsize len = 0;
    
    int32 it = 0;
    while (String_Decode(str, &it))
        ++len;
    
    return len;
}

#endif //INTERNAL_STRING_H
