# ============================================================
# Quest for the Chalice - Makefile
# ============================================================
# Targets:
#   make          -> build without Lua mod support
#   make lua      -> build with Lua 5.4 mod support (requires lua542Linux/)
#   make clean    -> remove build artifacts
# ============================================================

CC      = gcc
TARGET  = quest_for_chalice
SRC     = quest_for_chalice.c

# Raylib flags (assumes system install; adjust -L/-I if using local build)
RAYLIB_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Base build flags: optimize for size+speed, warn on everything, no exceptions
CFLAGS  = -O2 -Wall -Wextra -std=c99

# ---- Default build (no Lua) ----
all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(RAYLIB_FLAGS)
	@echo "Built $(TARGET) (no Lua mod support)"

# ---- Build with Lua 5.4.2 mod support ----
# Expects lua542Linux/include/ and lua542Linux/lib/ to exist
lua:
	$(CC) $(CFLAGS) -DUSE_LUA \
	    -Ilua542Linux/include \
	    $(SRC) -o $(TARGET) \
	    $(RAYLIB_FLAGS) \
	    -Llua542Linux/lib -llua -ldl
	@echo "Built $(TARGET) with Lua mod support"
	@echo "Place mod.lua in lua542Linux/ to customize rooms."

clean:
	rm -f $(TARGET)

.PHONY: all lua clean
