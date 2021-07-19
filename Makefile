CFLAGS =
LDFLAGS =

CUSTOMC ?=
CUSTOMLD ?=

CFLAGS += -Iinclude -Isrc
CFLAGS += -Wall -Werror-implicit-function-declaration -Wno-unused-function -Wconversion -Wno-format
CFLAGS += -Wno-missing-braces $(CUSTOMC)
LDFLAGS += -Llib $(CUSTOMLD)

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
	CLEAN = del "build\*.o" "build\*.pdb" "build\*.exp" "build\*.lib" "build\*.ilk"
	
	LDFLAGS += -luser32 -lgdi32 -lhid -static
	
	CFLAGS += -Wsizeof-array-decay -Werror=int-conversion -Werror=implicit-int-float-conversion
	CFLAGS += -Werror=float-conversion -Werror=sign-conversion -Wno-int-to-void-pointer-cast
	CFLAGS += -D_CRT_SECURE_NO_WARNINGS
	
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
	CLEAN = rm "build/*.o"
	
	ifeq ($(UNAME), Linux)
		PLATFORM = linux
		LDFLAGS += -lm -lX11 -ldl -lasound
	endif
	
	ifeq ($(UNAME), Darwin)
		PLATFORM = mac
	endif
endif

all: default
.PHONY: all default unity clean

unity: build/unity_build.o src/*.c src/*.h
	$(LD) $< -o "build/$(OUTPUT_NAME)" $(LDFLAGS)
default: build/engine.o build/platform_$(PLATFORM).o build/game.o
	$(LD) $^ -o "build/$(OUTPUT_NAME)" $(LDFLAGS)
clean:
	$(CLEAN)

build:
	mkdir build
build/%.o: src/%.c src/%*.c src/internal*.h | build
	$(CC) $< -c -o $@ $(CFLAGS)
src/unity_build.c:
