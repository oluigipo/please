#ifndef INTERNAL_LIST_H
#define INTERNAL_LIST_H

struct alignas(16) List_Header
{
    uintsize len;
    uintsize cap;
} typedef List_Header;

#define List__Header(x) ((x) ? (List_Header*)(x)-1 : NULL)
#define List__LengthL(x) List__Header(x)->len
#define List__CapL(x) List__Header(x)->cap
#define List_Length(x) ((x) ? List__Header(x)->len : 0)
#define List_Cap(x) ((x) ? List__Header(x)->cap : 0)

// Receives &list
#define List_Reserve(x, cap) (List__Reserve((void**)(x), sizeof *(*(x)), (cap)))
#define List_Push(x, value) (List_Reserve(x, List_Length(x)+1), (*(x))[List_Length(*(x))] = (value), List__LengthL(*(x))++)
#define List_Add(x) (List_Reserve(x, List_Length(x)+1), List__LengthL(*(x))++)
#define List_Pop(x) ((*(x))[--List__LengthL(*(x))])
#define List_Done(x) ((x) ? Engine_PopMemory(List__Header(x)) : 0, *(x) = NULL)

internal uintsize
List__Reserve(void** list, uintsize objsize, uintsize desired_cap)
{
	List_Header* header;
	
	if (!*list)
	{
		desired_cap = AlignUp(desired_cap, 15u);
		header = Engine_PushMemory(objsize * desired_cap + sizeof(*header));
		*list = (void*)(header + 1);
		
		return List_Cap(*list);
	}
	
	uintsize current_cap = List_Cap(*list);
	
	if (current_cap < desired_cap)
	{
		desired_cap = AlignUp(desired_cap, 15u);
		header = List__Header(*list);
		header->cap = desired_cap;
		
		List_Header* new_header = Engine_PushMemory(objsize * desired_cap + sizeof(*header));
        memcpy(new_header, header, header->len * objsize + sizeof(*header));
        Engine_PopMemory(header);
        header = new_header;
        
		*list = (void*)(header + 1);
	}
	
	return List_Cap(*list);
}

#endif //INTERNAL_LIST_H
