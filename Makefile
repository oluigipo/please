ifndef $(PLATFORM)
	ifeq ($(OS), Windows_NT)
		PLATFORM = win32
	else
		UNAME := $(shell uname -s)
		
		ifeq ($(UNAME), Linux)
			PLATFORM = linux
		else ifeq ($(UNAME), Darwin)
			PLATFORM = mac
		endif
	endif
endif

ifndef $(DEBUG)
	CUSTOMC ?= -O2 -ffast-math
else
	CUSTOMC ?=
endif
CUSTOMLD ?=
DEBUG ?=
BUILDDIR ?= build

CFLAGS += -Wall -Werror-implicit-function-declaration -Wno-unused-function -Wconversion -Wno-format
CFLAGS += -Wno-missing-braces -Wno-sign-conversion
CFLAGS += -Iinclude $(DEBUG) $(CUSTOMC)
LDFLAGS += -Llib $(DEBUG) $(CUSTOMLD)

ifeq ($(PLATFORM), win32)
	OUTPUT_NAME = game.exe
	CLEAN = del "build\*.o" "build\*.pdb" "build\*.exp" "build\*.lib" "build\*.ilk"
	
	CC = clang -std=c99
	LD = clang -fuse-ld=lld
	
	LDFLAGS += -luser32 -lgdi32 -lhid -static -Xlinker /subsystem:windows
	
	CFLAGS += -Wsizeof-array-decay -Werror=implicit-int-float-conversion
	CFLAGS += -Werror=float-conversion -Wno-int-to-void-pointer-cast
else ifeq ($(PLATFORM), linux)
	OUTPUT_NAME = game
	CLEAN = rm ./build/*.o
	
	CC = clang -std=c99
	LD = clang -fuse-ld=lld
	
	LDFLAGS += -lm -lX11 -ldl -lasound
endif

all: default
.PHONY: all default unity clean src/unity_build.c

unity: $(BUILDDIR)/unity_build.o
	$(LD) $^ -o "$(BUILDDIR)/$(OUTPUT_NAME)" $(LDFLAGS)
default: $(BUILDDIR)/engine.o $(BUILDDIR)/platform_$(PLATFORM).o $(BUILDDIR)/game.o
	$(LD) $^ -o "$(BUILDDIR)/$(OUTPUT_NAME)" $(LDFLAGS)
clean:
	$(CLEAN)

$(BUILDDIR):
	mkdir $(BUILDDIR)
$(BUILDDIR)/%.o: src/%.c src/%*.c src/internal*.h | $(BUILDDIR)
	$(CC) $< -c -o $@ $(CFLAGS)
