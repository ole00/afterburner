#!/bin/bash

set -e
# specify name of your PLD design and device to run on
APP=counter
DEV=ATF1502AS
PKG=PLCC44
SPD=15

## compile Verilog design by yosys
#../../../yosys/atf15xx_yosys/run_yosys.sh $APP > $APP.log


## use Atmel fitter to produce .jed file
#../../yosys/atf15xx_yosys/run_fitter.sh -d $DEV -p $PKG -s $SPD $APP -preassign keep -tdi_pullup on -tms_pullup on -output_fast off -xor_synthesis on  $*

# convert jed to svf
python3 ./fuseconv.py -d $DEV $APP.jed $APP.svf

# convert svf to xsvf
python3 .//svf2xsvf.py $APP.svf $APP.xsvf

date
echo "done!"