#include "common.h"
#include <stdio.h>
#include <stdlib.h>

#include "util_sdlgamecontroller.h"

static void
PrintHelp(const char* self)
{
	fprintf(stderr, "usage: %s path/to/gamecontrollerdb.txt path/to/output.h\n", self);
}

static int32
AsHexChar(uint8 c)
{
	if (c >= 'a')
		return c - 'a' + 10;
	if (c >= 'A')
		return c - 'A' + 10;
	return c - '0';
}

struct Writing
{
	char* buf;
	uintsize len;
	Arena* arena;
}
typedef Writing;

static Writing
BeginWriting(Arena* arena)
{
	return (Writing) {
		.buf = Arena_End(arena),
		.arena = arena,
		.len = 0,
	};
}

static void
Write(Writing* w, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	String str = Arena_VPrintf(w->arena, fmt, args);
	w->len += str.size;
	
	va_end(args);
}

static void
DoneWriting(Writing* w, FILE* file)
{
	fwrite(w->buf, 1, w->len, file);
	Arena_Pop(w->arena, w->buf);
	w->len = 0;
}

static void
Process(Arena* arena, String input_data, const char* input_name, FILE* outfile)
{
	static const String objects_table[] = {
		[USdldb_Button_A] = StrInit("GamepadObject_Button_A"),
		[USdldb_Button_B] = StrInit("GamepadObject_Button_B"),
		[USdldb_Button_X] = StrInit("GamepadObject_Button_X"),
		[USdldb_Button_Y] = StrInit("GamepadObject_Button_Y"),
		[USdldb_Button_Left] = StrInit("GamepadObject_Button_Left"),
		[USdldb_Button_Right] = StrInit("GamepadObject_Button_Right"),
		[USdldb_Button_Up] = StrInit("GamepadObject_Button_Up"),
		[USdldb_Button_Down] = StrInit("GamepadObject_Button_Down"),
		[USdldb_Button_LeftShoulder] = StrInit("GamepadObject_Button_LeftShoulder"),
		[USdldb_Button_RightShoulder] = StrInit("GamepadObject_Button_RightShoulder"),
		[USdldb_Button_LeftStick] = StrInit("GamepadObject_Button_LeftStick"),
		[USdldb_Button_RightStick] = StrInit("GamepadObject_Button_RightStick"),
		[USdldb_Button_Start] = StrInit("GamepadObject_Button_Start"),
		[USdldb_Button_Back] = StrInit("GamepadObject_Button_Back"),
		
		[USdldb_Pressure_LeftTrigger] = StrInit("GamepadObject_Pressure_LeftTrigger"),
		[USdldb_Pressure_RightTrigger] = StrInit("GamepadObject_Pressure_RightTrigger"),
		
		[USdldb_Axis_LeftX] = StrInit("GamepadObject_Axis_LeftX"),
		[USdldb_Axis_LeftY] = StrInit("GamepadObject_Axis_LeftY"),
		[USdldb_Axis_RightX] = StrInit("GamepadObject_Axis_RightX"),
		[USdldb_Axis_RightY] = StrInit("GamepadObject_Axis_RightY"),
	};
	
	Writing* w = &(Writing) { 0 };
	*w = BeginWriting(arena);
	
	Write(w, "// This file was generated from \"%s\"\n\n", input_name);
	Write(w, "static const GamepadMappings global_gamepadmap_database[] = {\n");
	
	const uint8* head = input_data.data;
	const uint8* const end = input_data.data + input_data.size;
	
	while (head < end)
	{
		while (*head == '\n')
			++head;
		if (!*head)
			break;
		if (*head == '#')
		{
			const uint8* found = Mem_FindByte(head, '\n', end - head);
			if (!found)
				break;
			
			head = found + 1;
			continue;
		}
		
		const uint8* eol = Mem_FindByte(head, '\n', end - head);
		if (!eol)
			eol = end - 1;
		
		String line = {
			.size = eol - head,
			.data = head,
		};
		
		USdldb_Controller con;
		String platform;
		if (USdldb_ParseEntry(line, &con, &platform) && String_Equals(platform, Str("Windows")))
		{
			Write(w, "\t//%S\n\t{", con.name);
			
			// Print GUID
			Write(w, ".guid={");
			for (int32 i = 0; i < ArrayLength(con.guid); ++i)
			{
				Write(w, "'%c',", con.guid[i]);
			}
			Write(w, "},");
			
			// Print buttons
			for (int32 i = 0; i < ArrayLength(con.buttons); ++i)
			{
				USdldb_Object obj = con.buttons[i];
				
				if (obj != 0)
					Write(w, ".buttons[%i]=%S,", i, objects_table[obj]);
			}
			
			// Print axes
			for (int32 i = 0; i < ArrayLength(con.axes); ++i)
			{
				USdldb_Object obj = con.axes[i];
				
				if (obj != 0)
					Write(w, ".axes[%i]=%S|%i,", i, objects_table[obj & 0xff], obj & 0xff00);
			}
			
			// Print povs
			for (int32 i = 0; i < ArrayLength(con.povs); ++i)
			{
				for (int32 k = 0; k < ArrayLength(con.povs[i]); ++k)
				{
					USdldb_Object obj = con.povs[i][k];
					
					if (obj != 0)
						Write(w, ".povs[%i][%i]=%S,", i, k, objects_table[obj]);
				}
			}
			
			Write(w, "},\n");
		}
		
		head = eol + 1;
	}
	
	Write(w, "};\n");
	DoneWriting(w, outfile);
}

int
main(int argc, char* argv[])
{
	if (argc < 3)
	{
		PrintHelp(argv[0]);
		return 1;
	}
	
	const char* input_fname = argv[1];
	const char* output_fname = argv[2];
	
	Arena* arena = Arena_Create(32 << 20, 8 << 20);
	String input_data;
	FILE* outfile;
	
	// NOTE(ljre): Try to read whole input file.
	{
		FILE* file = fopen(input_fname, "rb");
		if (!file)
		{
			fprintf(stderr, "could not open input file \"%s\".\n", input_fname);
			return 1;
		}
		
		fseek(file, 0, SEEK_END);
		uintsize size = ftell(file);
		rewind(file);
		
		char* buf = Arena_PushAligned(arena, size + 1, 1);
		if (fread(buf, 1, size, file) != size)
		{
			fprintf(stderr, "could not read whole input file.\n");
			return 1;
		}
		
		buf[size] = 0;
		fclose(file);
		
		input_data = StrMake(size+1, buf);
	}
	
	// NOTE(ljre): Try to open output file.
	{
		FILE* file = fopen(output_fname, "wb");
		if (!file)
		{
			fprintf(stderr, "could not open output file \"%s\".\n", output_fname);
			return 1;
		}
		
		outfile = file;
	}
	
	Process(arena, input_data, input_fname, outfile);
	fclose(outfile);
	return 0;
}
