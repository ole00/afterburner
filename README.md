# afterburner
GAL chip programmer for Arduino

This is a GAL IC programmer software that allows to program GAL IC chips
from various manfucaturers. It is based on work of several other people:

 Bruce Abbott: 
     http://www.bhabbott.net.nz/atfblast.html

 Manfred Winterhoff: 
     http://www.armory.com/%7Erstevew/Public/Pgmrs/GAL/_ClikMe1st.htm

who did the most of the hard work of deciphering and publishing the programming
protocol of these chips. Their programs were Windows based and relied on
presence of parallel port (LPT). Afterburner was written for Linux OS, and
requires serial connection to Arduio UNO, which does the programming
of the chip.

Supported GAL chips:

* Atmel ATF16V8B, ATF22V10B, ATF22V10CQZ 
* Lattice GAL16V8A, GAL16V8B, GAL16V8D
* Lattice GAL22V10B
* National GAL16V8

Setup:
* Upload the afterburner.ino sketch to your Arduino UNO.

* Connect the pins of the GAL chip to Arduino UNO according to
  the schematics images. Ideally use a PCB (ether etched or made
  in a fab) that's provided in 'pcb' and 'gerbers' directory.
  You can also hardwire on protoboard or breadboard.

* Compile the afteburner.c to get afterburner executable. Run
  ./compile.sh to do that.

* Set the programming voltage (VPP) on the voltage up-converter
  module (MT3608) depending on the GAL chip 
  Atmel ATF16V8B - 10V
  Lattice GAL16V8 - 10V, also works with 12V
  Others - between 10 - 14V - untested 

* Check the programming voltage (VPP) without the GAL chip being
  inserted / connected to Arduino UNO. Test the voltage on MT3608
  module VOUT- and VOUT+ pins while running the following command:
  <pre>
  ./afterburner s
  </pre>
  
* Check the chip identification by runnig the following command:
  <pre>
  ./afterburner i
  </pre>

  If you get some meaningfull GAL chip identification like:
  <pre>
  PES info: Atmel ATF16V8B  VPP=10.0 Timing: prog=10 erase=25
  </pre>
  then all should be well and you can try to  erase the chip and then
  programm it to contain your .jed file.

  If you get an unknown chip identification like:
  <pre>
  PES info: 3.3V Unknown GAL,  try VPP=10..14 in 1V steps
  </pre>
  then look at the troubleshooting section

* Read the content fo your GAL chip. This only works if the contents
  of the chip is not protected. Use the following command:
  <pre>
  ./afterburner r -t [GAL_type]
  </pre>
  or to save the printed .jed fuse map  to a file use:
  <pre>
  ./afterburner r -t [GAL_type] > my_gal.jed
  </pre>
  
* Erase the GAL chip. Before writing / programming the chip it must
  be erased - even if it is a branch new chip that has not been used 
  before. Use the following command:
  <pre>
  ./afterburner e -t [GAL type]
  </pre>

* Program and verify the GAL chip via the following command:
  <pre>
  ./afterburner wv -t [GAL type] -f my_new_gal.jed
  </pre>

How aferburner works:
---------------------
* PC code reads and parses .jad files, then produces a binary which
  can be the uploaded to Arduino via serial port. By default
  /dev/ttyUSB0 is used, but that can be changed to any other device
  by passing the following option to afterburner:
  <pre>
  -d /my/serial/device
  </pre>

* PC code of afterburner communicates with Arduino UNO's afterburner
  sketch by a trivial text based protocol. You can also connect directly
  to Arduino UNO via serial terminal of your choice and issue some basic
  commands manually.

* Arduino UNO's afterburner sketch does 2 things: 
  1) parses commands and data sent from the PC afterburner code
  2) toggles the GPIO pins and drives programming of the GAL content

* more information about GAL chips and their programming can be found here:

  http://www.bhabbott.net.nz/atfblast.html
  
  http://www.armory.com/%7Erstevew/Public/Pgmrs/GAL/_ClikMe1st.htm 


Troubleshooting:
----------------
- it does not work!

  * double check the schematics and the connection between Arduino UNO and
  the GAL chip.
  
  * ensure the GAL chip is inserted to the IC socket the right way (check 
  the pin 1 location)
  
  * ensure the VPP is set correctly on the MT3608 module. If unsure which
  voltage to use then try individual voltages one by one:
  10V, 11V, 12V, 13V, 14V, 15V. Do not go beyond 15V as you may damage the 
  GAL chip.
  
  * measure the VPP only when the chip is physically DISCONNECTED. Some brands
  of the GAL chip (Atmel) - when connected - iternally lower the voltage
  on the Edit (VPP) pin (voltage divider?) and such voltage reading is misleading.
  
  * use an external power supply for your Arduino UNO, powering
  just via serial USB cable may not be sufficient for driving the GAL chip and the
  voltage up-converter


- afterburner reports it can not connect to /dev/ttyUSB0, permission denied

  * ensure your user is member of the of the dialup group or alernatively run
  afterburner  as superuser (use: sudo ./afterburner ...)

- afterburner fails to connect to /dev/ttyUSB0

  * ensure your PC is actually connected to Arduino UNO, check a serial device
  exists:
  <pre>
  ls -alF /dev/ttyUSB*
  </pre>

- my MT3608 module does not have EN pin

  * that's unfortanetly a common problem. You have to mod the module as follows:

  * cut the PCB trace between pin 4 & 5 of the MT3608 chip. Verify the trace
  is cut by continuity probe (no beep is audible) between pins 4 & 5.

  <pre>
  |  Blue Potentiometer     |
  |_________________________|
  |   ______________
  |  |              |
  |  | SMT Capacit. |
  |  |______________|
  |
  |   3  2  1
  |   |  |  |
  |  +-------+
  |  |       |
  |  |4  5   |
  |  +-------+
  |   |X |  |
  |
  |<-- left long edge of the module
  </pre> 

  * X marks the spot - here make the vertical cut. Do not cut the pins
  itself! Just cut the trace between the pins, which is only barely visible
  without magnification. 
  ![See the MT3608 image in img directory for more details.] (https://github.com/ole00/afterburner/raw/master/img/mt3608_mod.jpg "EN pin mod")
  
    
  * solder a thin keynar wire to pin 4 which is your ENable pin.

  * Once that is done, toggling the EN pin will change the output voltage
  of the module between 5V (actually slightly less than that) and the
  voltage set by the potentiometer (10V and more).

- my MT3608 module does not seem to work - turning the pot does nothing

  * The pot needs to be turned at least 10 revolutions counter-clockwise
  to do anything useful. Keep trying.

- where to get the MT3608 module ?

  * usual places: ebay, aliexpress

- what are the .jed files and how to produce them
  
  * Use WinCupl software from Atmel. Works under Wine as well.
  
  https://www.microchip.com/design-centers/programmable-logic/spld-cpld/tools/software/wincupl
