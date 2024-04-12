JTAG conversion tools
---------------------------------------

fuseconv.py : converts ATF150X .jed file into .svf file

   The origin of the conversion tool is this git repo:
   https://github.com/whitequark/prjbureau


svf2xsvf.py : converts .svf file into .xsvf file 

  The origin of the tool is this git repo:
  https://github.com/arduino/OpenOCD/tree/master/contrib/xsvf_tools


Typically you produce a .jed file for your ATF150X device either by
WinCUPL or by ATF15XX_Yosys (https://github.com/hoglet67/atf15xx_yosys/)
and then run fuseconv.py and svf2xsvf.py to produce .xsvf file that can be
used by Aftereburner's JTAG player.

See example_jed2xsvf.sh for more information how to run these tools.

