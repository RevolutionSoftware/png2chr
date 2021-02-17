#!/bin/sh

BIN=png2nes
SOURCE=png2nes.c
BINDBG=${BIN}dbg
CFLAGS="-std=c89 -Wall -Wno-unknown-pragmas"
LDFLAGS="-L/usr/local/lib -lpng"
DEBUG_FLAGS="${CFLAGS} -DDEBUG -Wpedantic -Wshadow -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -Wvla -g -Og -fsanitize=address -fsanitize=undefined"
RELEASE_FLAGS="${CFLAGS} -Os -DNDEBUG -g0 -s"

# Debug (slow)
build_debug()
{
    cc $SOURCE $DEBUG_FLAGS $LDFLAGS -o $BINDBG
}

# Build (fast)
build_release()
{
    cc $SOURCE $RELEASE_FLAGS $LDFLAGS -o $BIN
}

# Build Debug but pipe binary into /dev/null
build_test()
{
    cc $SOURCE $DEBUG_FLAGS $LDFLAGS -o /dev/null
}

cleanup()
{
    rm -f $BIN $BINDBG
}

code_format()
{
    clang-format -i $SOURCE
}

case "$1" in
    d)
	build_debug
	;;
    r)
	build_release
	;;
    c)
	cleanup
	;;
    f)
	code_format
	;;
    dr|rd)
	build_release
	build_debug
	;;
    t)
	build_test
	;;
    *)
	echo "Usage: $0 {d|r|c|f}"
        echo ""
        echo "This shell script builds $BIN."
	echo "[r]elease mode"
	echo "[d]ebug mode"
	echo "[c]lean away the binaries"
	echo "[f]ormat the code with clang-format"
	echo ""
	echo "HINT: You can combine the d and r directives."
esac

#echo "Size: $(du -sk ./${BIN})"
