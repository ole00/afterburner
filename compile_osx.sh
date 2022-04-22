# path to your Osx cross-compiler
CC=~/opt/osxcross/bin/o64-clang

$CC -g3 -O0 -DNO_CLOSE -o afterburner_osx  src_pc/afterburner.c
