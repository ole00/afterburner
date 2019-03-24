# path to your win32 cross-compiler
CC=~/opt/mingw32/bin/i686-w64-mingw32-gcc

$CC -g3 -O0 -o afterburner_w32.exe afterburner.c -D_USE_WIN_API_
