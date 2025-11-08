# path to your Osx cross-compiler (https://github.com/tpoechtrager/osxcross)
export PATH=~/dev/osxcross2/bin:$PATH
CC=arm64-apple-darwin24.4-clang

GCOM=`git  rev-parse --short HEAD`


$CC -g3 -O0  -D_OSX_ -DNO_CLOSE -DGCOM="\"g${GCOM}\"" -o afterburner_osx_arm  src_pc/afterburner.c src_pc/exerciser.c
