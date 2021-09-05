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

CUSTOMC ?=
CUSTOMLD ?=
OPT ?= -O2 -ffast-math -flto
BUILDDIR ?= build

CFLAGS += -Iinclude
CFLAGS += -Wall -Werror-implicit-function-declaration -Wno-unused-function -Wconversion -Wno-format
CFLAGS += -Wno-missing-braces $(CUSTOMC) $(OPT)
LDFLAGS += -Llib $(CUSTOMLD) $(OPT)

ifeq ($(PLATFORM), win32)
	OUTPUT_NAME = game.exe
	CLEAN = del "build\*.o" "build\*.pdb" "build\*.exp" "build\*.lib" "build\*.ilk"
	
	CC = clang -std=c99
	LD = clang -fuse-ld=lld-link
	
	LDFLAGS += -luser32 -lgdi32 -lhid -static -Xlinker /subsystem:windows
	
	CFLAGS += -Wsizeof-array-decay -Werror=int-conversion -Werror=implicit-int-float-conversion
	CFLAGS += -Werror=float-conversion -Werror=sign-conversion -Wno-int-to-void-pointer-cast
else ifeq ($(PLATFORM), linux)
	OUTPUT_NAME = game
	CLEAN = rm ./build/*.o
	
	CC = cc
	LD = cc
	
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
