CC = clang
STD = -std=c99
GAME_NAME = womm

# Detect OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	PLATFORM_LIBS = -lm -ldl -lrt -lX11
else ifeq ($(OS),Windows_NT)
	PLATFORM_LIBS = -lm -luser32 -lgdi32 -lkernel32
else
    $(error Unsupported OS)
endif

MODE ?= debug
DEBUG_FLAGS = -g -MD
RELEASE_FLAGS = -O3

ifeq ($(MODE),debug)
	DEFINES += -DDEBUG -DASSERT_ENABLE
	COMMONS = $(DEBUG_FLAGS)
else ifeq ($(MODE),release)
	DEFINES += -D_RELEASE
	COMMONS = $(RELEASE_FLAGS)
else
	$(error Unknown build mode: $(MODE))
endif

# Add all subdirs in src to include path (recursive)
WARNINGS = -Wall -Wextra -Wno-c2x-extensions -Wpointer-arith -Wcast-align -Wconversion \
		   -Wstrict-aliasing -Wno-gnu-zero-variadic-macro-arguments
INCLUDES = -Isrc -I$(VULKAN_SDK)/include

# Final compiler flags
CFLAGS = $(STD) $(WARNINGS) $(INCLUDES) $(COMMONS) $(DEFINES)

# Source and object files
SRC = $(shell find src -name '*.c')
OBJ = $(SRC:%.c=obj/%.o)
DEP = $(OBJ:.o=.d)

TARGET = bin/$(GAME_NAME)

# Build
$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	@echo "Building $@"
	@$(CC) -o $@ $(OBJ) $(PLATFORM_LIBS)

# Rule for building object files in obj/ folder
obj/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	@echo "Cleaning..."
	@rm -rf obj bin

# Include dependency files
-include $(DEP)
