#========= Basic Variables
# The variables CUSTOMC and CUSTOMLD will be appended to CFLAGS and LDFLAGS.

ifndef DEBUG
	CUSTOMC ?= -O2 -ffast-math
else
	CUSTOMC ?=
endif
CUSTOMLD ?=
DEBUG ?=
BUILDDIR ?= build

CFLAGS += -Wall -Werror-implicit-function-declaration -Wno-unused-function -Wconversion -Wno-format
CFLAGS += -Wno-missing-braces -Wno-sign-conversion
CFLAGS += -std=gnu99 -Iinclude $(DEBUG) $(CUSTOMC)
LDFLAGS += -Llib $(DEBUG) $(CUSTOMLD)

# Compiler and linker
CC = clang
LD = $(CC)
ifeq ($(LD), clang)
	LD += -fuse-ld=lld
endif

#========= Recognize Platform
ifndef PLATFORM
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

#========= Translation Units
# You can set this to 'unity_build' if you want to
TUS ?= engine game platform_$(PLATFORM)

OBJS = $(TUS:%=$(BUILDDIR)/%.o)

#========= Platform Dependent Variables
ifeq ($(PLATFORM), win32)
	OUTPUT_NAME = game.exe
	EXTS_ = o pdb exp lib ilk dll
	CLEAN = del $(EXTS_:%="$(BUILDDIR)\*.%")
	
	LDFLAGS += -luser32 -lgdi32 -lhid -static -Wl,/subsystem:windows
	
	CFLAGS += -Wsizeof-array-decay -Werror=implicit-int-float-conversion
	CFLAGS += -Werror=float-conversion -Wno-int-to-void-pointer-cast
else ifeq ($(PLATFORM), linux)
	OUTPUT_NAME = game
	CLEAN = rm ./build/*.o
	
	LDFLAGS += -lm -lX11 -ldl -lasound
endif

#========= Rules
all: game
objs: $(OBJS)
.PHONY: all objs game clean src/unity_build.c

game: $(OBJS)
	$(LD) $^ -o "$(BUILDDIR)/$(OUTPUT_NAME)" $(LDFLAGS)
clean:
	$(CLEAN)

$(BUILDDIR):
	mkdir $(BUILDDIR)
$(BUILDDIR)/%.o: src/%.c src/%*.c src/internal*.h | $(BUILDDIR)
	$(CC) $< -c -o $@ $(CFLAGS)
$(BUILDDIR)/game.o: src/game.c src/game*.c src/internal*.h src/system*.c | $(BUILDDIR)
	$(CC) src/game.c -c -o $@ $(CFLAGS)
