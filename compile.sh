
GCOM=`git  rev-parse --short HEAD`


gcc -g3 -O0 -DNO_CLOSE -DGCOM="\"g${GCOM}\"" -o afterburner src_pc/afterburner.c
