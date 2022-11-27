#ifndef QOI_H
#define QOI_H

static int32
Qoi_HashOfPixel_(uint32 color)
{
	uint8 r = (color >> 0 ) & 0xff;
	uint8 g = (color >> 8 ) & 0xff;
	uint8 b = (color >> 16) & 0xff;
	uint8 a = (color >> 24) & 0xff;
	
	return (r*3 + g*5 + b*7 + a*11) % 64;
}

//~ Internal API
static uint32*
Qoi_Parse(const uint8* data, uintsize size, Arena* arena, int32* out_width, int32* out_height)
{
	Trace();
	
	//- NOTE(ljre): Some types.
	struct QoiHeader
	{
		uint8 magic[4];
		uint32 width;
		uint32 height;
		uint8 channels;
		uint8 colorspace;
	};
	
	union QoiColor
	{
		struct
		{
			uint8 r, g, b, a;
		};
		
		uint8 array[4];
		uint32 value;
	};
	
	Assert(sizeof(union QoiColor) == sizeof(uint32));
	
	//- NOTE(ljre): Validate header.
	if (size < 14 + 8)
		return NULL;
	
	struct QoiHeader header = *(const struct QoiHeader*)data;
	header.width = Mem_ByteSwap32(header.width);
	header.height = Mem_ByteSwap32(header.height);
	
	if (Mem_Compare(header.magic, "qoif", 4) != 0 || // NOTE(ljre): Magic
		header.channels < 3 || header.channels > 4 || header.colorspace > 1 || // NOTE(ljre): Format limitations.
		header.colorspace != 1 || header.width > INT32_MAX || header.height > INT32_MAX) // NOTE(ljre): Our limitations.
	{
		return NULL;
	}
	
	//- NOTE(ljre): Init.
	const uint8* head = data + 14;
	const uint8* end = data + size;
	
	union QoiColor* colors = Arena_PushAligned(arena, header.width * header.height * 4, 4);
	uint32 color_index = 0;
	bool err = false;
	
	union QoiColor previous_pixel = { 0, 0, 0, 255 };
	union QoiColor pixels[64] = { 0 };
	
	//- NOTE(ljre): Decode.
	while (!err && color_index < header.width*header.height)
	{
		if (head >= end)
		{
			err = true;
			break;
		}
		
		uint8 top = head[0] >> 6;
		
		if (top == 0b00) // NOTE(ljre): QOI_OP_INDEX
		{
			uint8 index = *head++ & 0b00111111;
			previous_pixel = colors[color_index++] = pixels[index];
		}
		else if (top == 0b01) // NOTE(ljre): QOI_OP_DIFF
		{
			uint8 diff[4] = {
				((head[0] >> 4) & 0b11) - 2,
				((head[0] >> 2) & 0b11) - 2,
				((head[0] >> 0) & 0b11) - 2,
				0,
			};
			
			union QoiColor cur = previous_pixel;
			
			cur.array[0] += diff[0];
			cur.array[1] += diff[1];
			cur.array[2] += diff[2];
			cur.array[3] += diff[3];
			
			previous_pixel = colors[color_index++] = cur;
			++head;
		}
		else if (top == 0b10) // NOTE(ljre): QOI_OP_LUMA
		{
			if (head + 1 >= end)
			{
				err = true;
				break;
			}
			
			uint8 diff[4] = {
				head[1] >> 4,
				((head[0]) & 0b00111111) - 32,
				head[1] & 0b1111,
				0,
			};
			
			diff[0] += diff[1];
			diff[2] += diff[1];
			
			union QoiColor cur = previous_pixel;
			
			cur.array[0] += diff[0];
			cur.array[1] += diff[1];
			cur.array[2] += diff[2];
			cur.array[3] += diff[3];
			
			previous_pixel = colors[color_index++] = cur;
			++head;
		}
		else if (head[0] == 0b11111110) // NOTE(ljre): QOI_OP_RGB
		{
			if (head + 3 >= end)
			{
				err = true;
				break;
			}
			
			union QoiColor cur;
			++head;
			
			cur.array[0] = head[0];
			cur.array[1] = head[1];
			cur.array[2] = head[2];
			cur.array[3] = previous_pixel.array[3];
			
			previous_pixel = colors[color_index++] = cur;
			head += 3;
		}
		else if (head[0] == 0b11111111) // NOTE(ljre): QOI_OP_RGBA
		{
			if (head + 4 >= end)
			{
				err = true;
				break;
			}
			
			union QoiColor cur;
			++head;
			
			cur.array[0] = head[0];
			cur.array[1] = head[1];
			cur.array[2] = head[2];
			cur.array[3] = head[3];
			
			previous_pixel = colors[color_index++] = cur;
			head += 4;
		}
		else if (top == 0b11) // NOTE(ljre): QOI_OP_RUN
		{
			uint32 count = *head++ & 0b00111111;
			count += 1;
			
			Assert(count < 0b111111); // unreachable
			
			if (color_index + count > header.width*header.height)
			{
				err = true;
				break;
			}
			
			while (count --> 0)
				colors[color_index++] = previous_pixel;
		}
		else
		{
			err = true;
			break;
		}
		
		pixels[Qoi_HashOfPixel_(previous_pixel.value)] = previous_pixel;
	}
	
	//- NOTE(ljre): Check for end marker.
	if (head + 8 > end)
		err = true;
	else
	{
		const uint8 end_marker[8] = { 0,0,0,0, 0,0,0,1 };
		
		if (Mem_Compare(head, end_marker, 8) != 0)
			err = true;
	}
	
	//- NOTE(ljre): Check for errors and return.
	if (err)
	{
		Arena_Pop(arena, colors);
		return NULL;
	}
	
	*out_width = (int32)header.width;
	*out_height = (int32)header.height;
	
	return (uint32*)colors;
}

#endif //QOI_H
