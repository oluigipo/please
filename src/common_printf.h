#ifndef COMMON_PRINTF_H
#define COMMON_PRINTF_H

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include <ext/stb_sprintf.h>

internal uintsize
VSPrintf(char* buf, uintsize len, const char* fmt, va_list args)
{
	return (uintsize)stbsp_vsnprintf(buf, (int32)len, fmt, args);
}

internal uintsize
SPrintf(char* buf, uintsize len, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	uintsize result = VSPrintf(buf, len, fmt, args);
	
	va_end(args);
	return result;
}

internal uintsize
VSPrintfSize(const char* fmt, va_list args)
{
	return (uintsize)stbsp_vsnprintf(NULL, 0, fmt, args);
}

internal uintsize
SPrintfSize(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	uintsize result = VSPrintfSize(fmt, args);
	
	va_end(args);
	return result;
}

#endif //COMMON_PRINTF_H
