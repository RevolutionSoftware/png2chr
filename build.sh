#!/bin/sh

BIN=png2nes
SOURCE=png2nes.c
BINDBG=${BIN}dbg

# Debug (slow)
build_debug()
{
    cc -std=c89 -DDEBUG -Wall -Wno-unknown-pragmas -Wpedantic -Wshadow -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -Wvla -g -Og -fsanitize=address -fsanitize=undefined $SOURCE -L/usr/local/lib -lpng -o $BINDBG
}

# Build (fast)
build_release()
{
    cc $SOURCE -std=c89 -Os -DNDEBUG -g0 -s -Wall -Wno-unknown-pragmas -L/usr/local/lib -lpng -o $BIN
}

# Build Debug but pipe binary into /dev/null
build_test()
{
    cc -std=c89 -DDEBUG -Wall -Wno-unknown-pragmas -Wpedantic -Wshadow -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -Wvla -g -Og -fsanitize=address -fsanitize=undefined $SOURCE -L/usr/local/lib -lpng -c -o /dev/null
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
    br|rb)
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
