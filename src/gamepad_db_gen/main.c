#include "internal.h"
#include <stdio.h>
#include <stdlib.h>

internal void
PrintHelp(const char* self)
{
	fprintf(stderr, "usage: %s path/to/gamecontrollerdb.txt path/to/output.h\n", self);
}

internal void
MoveUntilEol(const char** phead)
{
	while (**phead && **phead != '\n')
		++*phead;
}

internal bool
IsHexChar(char c)
{
	return c >= '0' && c <= '9' || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F';
}

internal int32
AsHexChar(char c)
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

internal Writing
BeginWriting(Arena* arena)
{
	return (Writing) {
		.buf = Arena_End(arena),
		.arena = arena,
		.len = 0,
	};
}

internal void
Write(Writing* w, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	uintsize len;
	{
		va_list args2;
		va_copy(args2, args);
		len = vsnprintf(NULL, 0, fmt, args2);
		va_end(args2);
	}
	
	char* buf = Arena_PushAligned(w->arena, len, 1);
	len = vsnprintf(buf, len, fmt, args);
	
	w->len += len;
	
	va_end(args);
}

internal void
DoneWriting(Writing* w, FILE* file)
{
	fwrite(w->buf, 1, w->len, file);
}

internal void
Process(Arena* arena, const char* input_data, const char* input_name, FILE* outfile)
{
	fprintf(outfile, "// This file was generated from \"%s\"", input_name);
	fputs("internal const GamepadMappings global_gamepad_database[] = {\n", outfile);
	
	const char* head = input_data;
	
#define AssertCond(cond) do { if (!(cond)) goto done_parsing_line; } while (0)
	
	for (;;)
	{
		while (*head == '\n')
			++head;
		if (*head == '#')
			goto done_parsing_line;
		if (!*head)
			break;
		
		//- NOTE(ljre): Parse GUID
		char guid[32];
		for (int32 index = 0; index < ArrayLength(guid); ++index)
		{
			char ch = *head++;
			AssertCond(IsHexChar(ch));
			guid[index] = ch;
		}
		
		//- NOTE(ljre): Parse Name
		String name;
		
		//- NOTE(ljre): Write Entry. This part is reached when all data is good.
		{
			Writing w = BeginWriting(arena);
			Write(&w, "//%.*s\n", StrFmt(name));
			
			// GUID
			Write(&w, "{.guid={");
			for (int32 i = 0; i < ArrayLength(guid); ++i)
				Write(&w, "'%c',", guid[i]);
			Write(&w, "},");
			
			//
			
			Write(&w, "},");
			DoneWriting(&w, outfile);
		}
		
		done_parsing_line: MoveUntilEol(&head);
	}
	
#undef AssertCond
	
	fputs("};\n");
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		PrintHelp(argv[0]);
		return 1;
	}
	
	const char* input_fname = argv[1];
	const char* output_fname = argv[2];
	
	Arena* arena = Arena_Create(Megabytes(32), Megabytes(8));
	const char* input_data;
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
		
		input_data = buf;
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
