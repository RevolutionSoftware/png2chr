CC=gcc
CFLAGS=-std=c89 -Wall -Wno-unknown-pragmas
LDFLAGS=-L/usr/local/lib -lpng
SOURCE=png2nes.c
BIN=png2nes
BIN_DBG=png2nesdbg
DEBUG_FLAGS=$(CFLAGS) -DDEBUG -Wpedantic -Wshadow -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -Wvla -g -Og -fsanitize=address -fsanitize=undefined
RELEASE_FLAGS=$(CFLAGS) -Os -DNDEBUG -g0 -s

all: release

release:
	$(CC) $(SOURCE) $(RELEASE_FLAGS) $(LDFLAGS) -o $(BIN)

debug:
	$(CC) $(SOURCE) $(DEBUG_FLAGS) $(LDFLAGS) -o $(BIN_DBG)

test:
	$(CC) $(SOURCE) $(DEBUG_FLAGS) $(LDFLAGS) -o /dev/null

format_code:
	clang-format -i $(SOURCE)

clean:
	@rm -f $(BIN) $(BIN_DBG)
