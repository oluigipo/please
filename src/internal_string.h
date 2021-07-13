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
#define StrMacro_(x) #x
#define StrMacro(x) StrMacro_(x)
#define StrNull (String) { 0 }

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

internal inline int32
String_Compare(String a, String b)
{
    const char* ap = a.data;
    const char* bp = b.data;
    
    const char* ap_end = ap + a.len;
    const char* bp_end = bp + b.len;
    
    while (ap < ap_end && !ap_end[-1]) --ap_end;
    while (bp < bp_end && !bp_end[-1]) --bp_end;
    
    while (ap < ap_end && bp < bp_end)
    {
        if (*ap != *bp)
            return (int32)*ap - (int32)*bp;
        
        ++ap;
        ++bp;
    }
    
    int32 alen = (int32)(ap_end - ap);
    int32 blen = (int32)(bp_end - bp);
    
    if (alen != blen)
        return alen - blen;
    
    return 0;
}

internal inline bool32
String_EndsWith(String check, String s)
{
    if (s.len > check.len)
        return false;
    
    String substr = {
        .data = check.data + (check.len - s.len),
        .len = s.len,
    };
    
    return strncmp(substr.data, s.data, substr.len);
}

#endif //INTERNAL_STRING_H
