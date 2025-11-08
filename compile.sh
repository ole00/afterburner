
GCOM=`git  rev-parse --short HEAD`


gcc -g2 -O0 -DNO_CLOSE -DGCOM="\"g${GCOM}\"" -o afterburner src_pc/afterburner.c src_pc/exerciser.c
