CFLAGS =
LDFLAGS =

CUSTOMC ?=
CUSTOMLD ?=

CFLAGS += -I./include -I./src
CFLAGS += -Wall -Werror-implicit-function-declaration -Wno-unused-function -Wconversion -Wno-format
CFLAGS += -Wno-missing-braces $(CUSTOMC)
LDFLAGS += -L./lib $(CUSTOMLD)

OBJS = "./build/platform.o"
OBJS += "./build/engine.o"
OBJS += "./build/game.o"

ifeq ($(MODE), debug)
	CFLAGS += -g -DDEBUG
	LDFLAGS += -g
else ifeq ($(MODE), safedebug)
	CFLAGS += -g -DDEBUG -fsanitize=address
	LDFLAGS += -g -fsanitize=address
else
	OPT = -O1 -ffast-math -flto
	
	CFLAGS += $(OPT)
	LDFLAGS += $(OPT)
endif

ifeq ($(OS), Windows_NT)
	PLATFORM = win32
	OUTPUT_NAME = game.exe
	
	CC = clang -std=c99
	LD = clang -fuse-ld=lld-link
	
	LDFLAGS += -luser32 -lgdi32 -lhid -static
	
	CFLAGS += -Wsizeof-array-decay -Werror=int-conversion -Werror=implicit-int-float-conversion
	CFLAGS += -Werror=float-conversion -Werror=sign-conversion -Wno-int-to-void-pointer-cast
	CFLAGS += -D_CRT_SECURE_NO_WARNINGS -DOS_WINDOWS
	
	ifeq ($(ARCH), x86)
		CFLAGS += -m32 -DX86
		LDFLAGS += -m32 -Xlinker /subsystem:windows,5.01
	else
		CFLAGS += -m64 -DX64
		LDFLAGS += -m64 -Xlinker /subsystem:windows
	endif
else
	UNAME := $(shell uname -s)
	OUTPUT_NAME = game
	
	CC = clang -std=c99
	LD = clang
	
	ifeq ($(UNAME), Linux)
		PLATFORM = linux
		CFLAGS += -DOS_LINUX
		LDFLAGS += -lm -lX11 -ldl -lasound
	endif
	
	ifeq ($(UNAME), Darwin)
		PLATFORM = mac
		CFLAGS += -DOS_MAC
	endif
endif

all: build engine.o platform.o game.o
	$(LD) $(OBJS) -o "./build/$(OUTPUT_NAME)" $(LDFLAGS)

unity: CFLAGS += -DUNITY_BUILD
unity: build
	$(CC) "./src/unity_build.c" -o "./build/$(OUTPUT_NAME)" $(CFLAGS) $(LDFLAGS)

build:
	mkdir build

game: game.o
	$(LD) $(OBJS) -o "./build/$(OUTPUT_NAME)" $(LDFLAGS)

platform.o:
	$(CC) "./src/platform_$(PLATFORM).c" -c -o "./build/platform.o" $(CFLAGS)

engine.o:
	$(CC) "./src/engine.c" -c -o "./build/engine.o" $(CFLAGS)

game.o:
	$(CC) "./src/game.c" -c -o "./build/game.o" $(CFLAGS)
