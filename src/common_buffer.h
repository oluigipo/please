#ifndef COMMON_BUFFER_H
#define COMMON_BUFFER_H

struct Buffer
{
	uintsize size;
	const uint8* data;
}
typedef Buffer;

#ifndef __cplusplus
#   define Buf(x) (Buffer) BufInit(x)
#   define BufNull (Buffer) { 0 }
#   define BufMake(size, data) (Buffer) { (uintsize)(size), (const uint8*)(data) }
#   define BufRange(begin, end) (Buffer) BufInitRange(begin, end)
#else
#   define Buf(x) Buffer BufInit(x)
#   define BufNull Buffer {}
#   define BufMake(size, data) Buffer { (uintsize)(size), (const uint8*)(data) }
#   define BufRange(begin, end) Buffer BufInitRange(begin, end)
#endif

#define BufInit(x) { sizeof(x), (const uint8*)(x) }
#define BufInitRange(begin, end) { (uintsize)((end) - (begin)), (const uint8*)(begin) }

static inline int32
Buffer_Equals(Buffer a, Buffer b)
{
	if (a.size != b.size)
		return false;
	
	return Mem_Compare(a.data, b.data, a.size) == 0;
}

#endif //COMMON_BUFFER_H
