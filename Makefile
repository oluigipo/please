CC = clang
LD = clang -fuse-ld=lld-link

CFLAGS = -std=c99 -I./include -I./src
CFLAGS += -Wall -Werror-implicit-function-declaration -Wno-unused-function -Wconversion -Wnewline-eof
LDFLAGS = -L./lib

OBJS = "./build/platform.o"
OBJS += "./build/engine.o"
OBJS += "./build/game.o"

ifeq ($(MODE), debug)
	CFLAGS += -g -DDEBUG
	LDFLAGS += -g
else
	OPT = -O2 -fno-slp-vectorize -ffast-math -flto
	
	CFLAGS += $(OPT)
	LDFLAGS += $(OPT)
endif

ifeq ($(OS), Windows_NT)
	PLATFORM = win32
	OUTPUT_NAME = game.exe
	
	LDFLAGS += -luser32 -lgdi32 -lhid -ldinput8 -lole32 -loleaut32
	CFLAGS += -D_CRT_SECURE_NO_WARNINGS
	
	ifeq ($(MODE), x86)
		CFLAGS += -m32
		LDFLAGS += -m32 -Xlinker /subsystem:windows,5.01
	else
		CFLAGS += -m64
		LDFLAGS += -m64 -Xlinker /subsystem:windows
	endif
else
	UNAME := $(shell uname -s)
	OUTPUT_NAME = game
	
	ifeq ($(UNAME), Linux)
		PLATFORM = linux
	endif
	
	ifeq ($(UNAME), Darwin)
		PLATFORM = mac
	endif
endif

all: engine.o platform.o game.o
	$(LD) $(OBJS) -o "./build/$(OUTPUT_NAME)" $(LDFLAGS)

game: game.o
	$(LD) $(OBJS) -o "./build/$(OUTPUT_NAME)" $(LDFLAGS)

platform.o:
	$(CC) "./src/platform_$(PLATFORM).c" -c -o "./build/platform.o" $(CFLAGS)

engine.o:
	$(CC) "./src/engine.c" -c -o "./build/engine.o" $(CFLAGS)

game.o:
	$(CC) "./src/game.c" -c -o "./build/game.o" $(CFLAGS)
