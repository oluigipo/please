#ifndef UTIL_SDLGAMECONTROLLER_H
#define UTIL_SDLGAMECONTROLLER_H

uint16 typedef USdldb_Object;
enum USdldb_Object
{
	USdldb_Button_A = 1,
	USdldb_Button_B = 2,
	USdldb_Button_X = 3,
	USdldb_Button_Y = 4,
	USdldb_Button_Left = 5,
	USdldb_Button_Right = 6,
	USdldb_Button_Up = 7,
	USdldb_Button_Down = 8,
	USdldb_Button_LeftShoulder = 9, // LB
	USdldb_Button_RightShoulder = 10, // RB
	USdldb_Button_LeftStick = 11, // LS
	USdldb_Button_RightStick = 12, // RS
	USdldb_Button_Start = 13,
	USdldb_Button_Back = 14,
	
	USdldb_Pressure_LeftTrigger = 15, // LT
	USdldb_Pressure_RightTrigger = 16, // RT
	
	USdldb_Axis_LeftX = 17,
	USdldb_Axis_LeftY = 18,
	USdldb_Axis_RightX = 19,
	USdldb_Axis_RightY = 20,
	
	// The lower bits of this enum is one of these entries specified above.
	// The higher bits are used to store extra information about the object.
	//
	// Higher Bits: 0b7654_3210
	// 7 = If the axis should be inverted
	// 6 = If the input axis should be limited to 0..MAX
	// 5 = If the input axis should be limited to MIN..0
	// 4 = If the output axis should be limited to 0..MAX
	// 3 = If the output axis should be limited to MIN..0
	//
	// Bits 6 & 5 should never be set at the same time
	// Bits 4 & 3 should never be set at the same time
	
	USdldb_Count = 21,
};

struct USdldb_Controller
{
	uint8 guid[32];
	String name;
	
	USdldb_Object buttons[32];
	USdldb_Object axes[16];
	USdldb_Object povs[8][4];
}
typedef USdldb_Controller;

static bool USdldb_ParseEntry(String line, USdldb_Controller* out_controller, String* out_platform);

//- Implementation
static USdldb_Object
USdldb_FindObjectFromName_(String name)
{
	struct
	{
		String name;
		USdldb_Object object;
	}
	static const table[] = {
		{ StrInit("a"), USdldb_Button_A },
		{ StrInit("b"), USdldb_Button_B },
		{ StrInit("back"), USdldb_Button_Back },
		
		{ StrInit("dpdown"), USdldb_Button_Down },
		{ StrInit("dpleft"), USdldb_Button_Left },
		{ StrInit("dpright"), USdldb_Button_Right },
		{ StrInit("dpup"), USdldb_Button_Up },
		
		{ StrInit("leftshoulder"), USdldb_Button_LeftShoulder },
		{ StrInit("leftstick"), USdldb_Button_LeftStick },
		{ StrInit("lefttrigger"), USdldb_Pressure_LeftTrigger },
		{ StrInit("leftx"), USdldb_Axis_LeftX },
		{ StrInit("lefty"), USdldb_Axis_LeftY },
		
		{ StrInit("rightshoulder"), USdldb_Button_RightShoulder },
		{ StrInit("rightstick"), USdldb_Button_RightStick },
		{ StrInit("righttrigger"), USdldb_Pressure_RightTrigger },
		{ StrInit("rightx"), USdldb_Axis_RightX },
		{ StrInit("righty"), USdldb_Axis_RightY },
		
		{ StrInit("start"), USdldb_Button_Start },
		
		{ StrInit("x"), USdldb_Button_X },
		{ StrInit("y"), USdldb_Button_Y },
	};
	
	int32 left = 0;
	int32 right = ArrayLength(table);
	
	while (left < right)
	{
		int32 index = (left + right) / 2;
		int32 cmp = String_Compare(name, table[index].name);
		
		if (cmp < 0)
			right = index;
		else if (cmp > 0)
			left = index + 1;
		else
			return table[index].object;
	}
	
	return 0;
}

static bool
USdldb_ParseEntry(String line, USdldb_Controller* out_controller, String* out_platform)
{
	USdldb_Controller con = { 0 };
	String platform = StrNull;
	
	const uint8* const begin = line.data;
	const uint8* const end = line.data + line.size;
	const uint8* head = begin;
	
	if (line.size < 34 || begin[0] == '#')
		return false;
	
	//- GUID
	for (int32 i = 0; i < 32; ++i)
	{
		if (!(head[0] >= '0' && head[0] <= '9') && !(head[0] >= 'a' && head[0] <= 'f'))
			return false;
		
		con.guid[i] = *head++;
	}
	
	if (head >= end || *head++ != ',')
		return false;
	
	//- Name
	{
		const uint8* name_begin = head;
		const uint8* name_end = Mem_FindByte(head, ',', end - head);
		
		if (!name_end)
			return false;
		
		head = name_end;
		con.name = StrMake(name_end - name_begin, name_begin);
	}
	
	if (head >= end || *head++ != ',')
		return false;
	
	//- Mapping
	while (head < end)
	{
		// Left arg
		uint8 left_prefix = 0;
		if (head[0] == '+' || head[0] == '-')
			left_prefix = *head++;
		
		const uint8* left_arg_begin = head;
		while (head < end && head[0] != ':')
			++head;
		const uint8* left_arg_end = head;
		
		String left_arg = {
			.size = left_arg_end - left_arg_begin,
			.data = left_arg_begin
		};
		
		if (String_Equals(left_arg, Str("platform")))
		{
			if (head + 2 >= end || head[0] != ':')
				return false;
			
			const uint8* platform_begin = ++head;
			while (head < end && (head[0] != '\n' || head[0] != ','))
				++head;
			
			platform = StrRange(platform_begin, head - 1);
			
			if (head >= end || *head++ != ',')
				break;
			
			continue;
		}
		
		USdldb_Object object = USdldb_FindObjectFromName_(left_arg);
		if (object == 0)
			return false;
		
		if (head + 2 >= end || head[0] != ':')
			return false;
		++head; // eat ':'
		
		// Right arg
		uint8 right_prefix = 0;
		if (head[0] == '+' || head[0] == '-')
			right_prefix = *head++;
		
		uint8 kind = *head++;
		
		switch (kind)
		{
			case 'b':
			{
				int32 num = 0;
				while (head < end && head[0] >= '0' && head[0] <= '9')
				{
					num *= 10;
					num += *head++ - '0';
				}
				
				if (num >= ArrayLength(con.buttons) || con.buttons[num] != 0)
					return false;
				
				con.buttons[num] = object;
			} break;
			
			case 'h':
			{
				if (head + 3 >= end)
					return false;
				
				int32 left = *head++ - '0';
				if (*head++ != '.')
					return false;
				uint32 right = *head++ - '0';
				
				int32 bit = Mem_BitCtz32(right);
				
				if (left >= ArrayLength(con.povs) || bit >= 4 || con.povs[left][bit] != 0)
					return false;
				
				con.povs[left][bit] = object;
			} break;
			
			case 'a':
			{
				uint8 higher_bits = 0;
				
				if (left_prefix == '+')
					higher_bits |= 1 << 4;
				else if (left_prefix == '-')
					higher_bits |= 1 << 3;
				
				if (right_prefix == '+')
					higher_bits |= 1 << 6;
				else if (right_prefix == '-')
					higher_bits |= 1 << 5;
				
				int32 num = 0;
				while (head < end && head[0] >= '0' && head[0] <= '9')
				{
					num *= 10;
					num += *head++ - '0';
				}
				
				if (head < end && head[0] == '~')
					++head, higher_bits |= 1 << 7;
				
				if (num > ArrayLength(con.axes) || con.axes[num] != 0)
					return false;
				
				con.axes[num] = (uint16)(object | (higher_bits << 8));
			} break;
			
			default: return false;
		}
		
		if (head >= end || *head++ != ',')
			break;
	}
	
	//- Done
	*out_controller = con;
	*out_platform = platform;
	return true;
}

#endif //UTIL_SDLGAMECONTROLLER_H
