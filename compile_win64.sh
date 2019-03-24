# path to your Win64 cross-compiler
CC=~/opt/mingw64/bin/x86_64-w64-mingw32-gcc

$CC -g3 -O0 -o afterburner_w64.exe afterburner.c -D_USE_WIN_API_
