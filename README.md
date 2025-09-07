# afterburner
GAL chip programmer for Arduino
![Board image](https://github.com/ole00/afterburner/raw/master/img/afterburner_new_design.jpg "afterburner")

This is a GAL IC programmer software that allows to program GAL IC chips
from various manufcaturers. It is based on work of several other people:

 Bruce Abbott: 
     https://web.archive.org/web/20220121030038/http://www.bhabbott.net.nz/atfblast.html

 Manfred Winterhoff: 
     http://www.armory.com/%7Erstevew/Public/Pgmrs/GAL/_ClikMe1st.htm
 
 Yorck Thiele:
     https://www.ythiee.com/2021/06/06/galmate-hardware/

 Michael Dreher:
     https://github.com/nospam2000/afterburner.git
 
 Marcelo Roberto Jimenez: (JTAG player)
     https://github.com/mrjimenez/JTAG

 whitequark · it: (ATF150X jed to svf tool)
     https://github.com/whitequark/prjbureau

 OpenOCD: (svf to xsvf tool)
     https://github.com/arduino/OpenOCD/blob/master/contrib/xsvf_tools/svf2xsvf.py

who did the most of the hard work of deciphering and publishing the programming
protocol of these chips. Some of their early programs were Windows based and relied on
presence of parallel port (LPT). Afterburner was written for Linux OS 
(also works on Win32/64, Mac OSX64), and requires serial connection to
Arduino UNO, which does the programming of the GAL chip.

**Update: ver.0.6.0 added experimental support for ATF1502AS and ATF1504AS. Only
identify, erase and write commands are supported. Read function is unsupported.
Verification is usually done automatically - it is a part of the .xsvf JTAG file
which is used when writing the design. See Discussions for more information**

**Update: ver.0.5.8 improved calibration alogrithm and resolution for mcp4151 digi pot.
Please re-calibrate your Afterburner as the previsouly stored calibration data are invalid.**

Supported GAL chips:
---------------------

| | Atmel | Lattice | National | ST |
| --- | --- | --- | --- | --- |
| 16V8 | ATF16V8B, ATF16V8BQL, ATF16V8C | GAL16V8A, GAL16V8B, GAL16V8D | GAL16V8 | GAL16V8 |
| 18V10 | - | GAL18V10, GAL18V10B[1] | - | - |
| 20V8 | ATF20V8B | GAL20V8B | GAL20V8 | - |
| 20RA10 | - | GAL20RA10, GAL20RA10B | - | - |
| 20XV10 | - | GAL20XV10B | - | - |
| 22V10 | ATF22V10B, ATF22V10C, ATF22V10CQZ | GAL22V10B, GAL22V10D | - | - |
| 6001 | - | GAL6001B | - | - |
| 6002 | - | GAL6002B | - | - |
| 26CV12 | - | GAL26CV12B[2] | - | - |
| 26V12 | - | GAL26V12C[2] | - | - |
| 750 | ATF750C | - | - | - |
| 150X | ATF1502AS, ATF1502ASL, ATF1504AS, ATF1504ASL[2][3] | - | - | - |


[1]: requires PCB v.3.1 or modified PCB v.3.0 - see Troubleshooting  
[2]: requires adapter - see gerbers, pcb and img directory. PLCC-28 package is supported by a specific adapter - do not use a common PLCC-28 to
     DIP-24 adapter as it does not work for GAL26CV12B with Afterburner.  
[3]: also supports 3.3V ATF1502ASV and ATF1504ASV when Arduino IOREF is 3.3V (ARM or ESP32 based Arduinos or Arduinos with IOREF 3.3V switch)

[-]: - represents either this combination does not exist or hasn't been tested yet. Testers are welcome to report their findings.

**This is a new Afterburner design with variable programming voltage control and with single ZIF socket for 20 and 24 pin GAL chips.**
The PC software is backward compatible with the older Afterburner desgin/boards.
You can still access the older/simpler ![design here](https://github.com/ole00/afterburner/tree/version_4_legacy "Afterburner legacy")


The new design features:

* variable programming voltage control via digital potentiometer
* single 24 pin ZIF socket for 16V8, 20V8 and 22V10 GALs. The adapter for GAL20V8 is no longer needed.
* simpler connection to MT3608 module (no need to modify the module)
* both Through Hole and SMT footprints present on a single PCB. This allows
  to mix & match SMT and TH parts based on your skills and components availability.

Drawbacks compared to the old Afterburner design:

* more parts required, most notably the digital pot MCP4131 and a shift register 74hc595
* a few more steps during initial VPP calibration. But once the calibration is done it does not need to be changed for different GAL chips.
* the PCB design for etched  board is no longer provided because of the higher complexity. Please use a PCB fabrication service or use the older Afterburner design (see above).


Setup:
---------------------

* Upload the afterburner.ino sketch to your Arduino UNO. Use Arduino IDE to upload the sketch, both IDE version 1.8.X and 2.X should work.

* Build the Afterburner hardware. Buy the PCB from the an online PCB production service (use provided gerber archive in 'gerbers' directory). Then solder the components on the PCB - check the schematic.pdf and BOM.txt for parts list.

* Compile the afteburner.c to get afterburner executable. Run
  ./compile.sh to do that. Alternatively use the precompiled binaries in the 'releases' directory.

* Calibrate the variable voltage. This needs to be done only once, before you start using Afterburner for programming GAL chips.
  Calibration procedure differs a little bit when using MT3608 module or when using on board voltage booster.

  **When using MT3608 module**

  * **Calibration step 1)** Turn the small potentiometer (R9) on the Afterburner to the middle position. This pot acts as compensation resistor for the digital pot.

  * **Calibration step 2)** Set the programming voltage (VPP) to 16.5V: Check the programming voltage (VPP) without the GAL chip being inserted / connected to Afterburner. Test the voltage on MT3608 module VOUT- and VOUT+ pins while running the following command:
  <pre>
  ./afterburner s
  </pre>
  While the command is running turn the pot on the MT3608 module (not the Afterburner's pot) counter clockwise (5 to 20 full turns). The VPP voltage should be displayed on the console, but for the very first setup use a multimeter to verify the VPP voltage as well.
  Re-run the command (if needed) to give you more time to set the 16.5V VPP.

  * **Calibration step 3)** This step scans the available voltages and records the pot taps. Run the following command:
  <pre>
  ./afterburner b
  </pre>
  You will see several messages on the console. Check the one with '*Index for VPP 900 is'. This is the lowest supported VPP of 9V and the index should ideally be between 30 and 70, but if it a bit less or a bit higher (let's say from 20 to 90) the calibration should still be valid. 
  If you see a significantly different index value (lower or higher) move the Afterburner's compensation pot (R9) either a bit lower or higher (depending on the VPP 900 index value) and go back to Calibration step 2). Repeat the Calibration steps 2) and 3) until you find the good value on VPP 900 index. If everything goes OK the last VPP index (VPP 1650) should be 255. If it is not exactly 255, but at least above 250 then it is fine. 

  * **Calibration step 4)** Measure the actual VPP to verify the value read by Arduino is correct. Run the following command while measuring the VPP on your multimeter:
  <pre>
  ./afterburner m
  </pre>
  Afterburner will set the VPP to several values (5V, 9V, 12V, 14V, 16V) and print the voltage readings as read by Arduinos's ADC. These values should match with readings from your multimeter (except for the 5V which is OK if it is a value from 4.2V - 5.0 V). Important are the values of the higher voltages. If they are off by more than +/-0.05V then you can set the calibration offset by running Calibration step 3) with an extra parameter '-co X':
  <pre>
  ./afterburner b -co X
  </pre>
  Where X is a number from -32 (representing -0.32V offset) to value 32 (representing +0.32V offset). If your multimeter reads 12.1V and the reading on the text console shows 12.00V you need to set positive offset of +0.1V ('-co 10'). If your multimeter reads 11.85V and the reading on the text console shows 12.05V you need to set negative offset of -0.2V ('-co -20'). After setting the calibration offset, the readings on your multimeter should ideally read the same values as the text on the console (+/- 0.05V). The calibration is then done. If (when specifying negative offset value) the calibration fails, turn the MT3608 Pot about 10-15 degrees counter-clockwise (to rise the VPP a tiny bit) and re-do the Calibration step 3.

  * Note that if you use your calibrated Afterburner board with a different Arduino (made by a different company or slightly different design), you may need to re-do the calibration.

**When using on-board voltage booster:**

Calibration steps are the same as for MT3608 module with these differences

* **Calibration Step 2)** We can't turn the extra pot on the MT3608 module, but we can
adjust the voltage by turning the R9 pot on the Afterburner PCB. It's OK to have
the voltage a bit higher like 16.6V or so in this calibration step.

* **Calibration Step 3)** Because of the feedback resistance difference compared to MT3608
module the calibration index for 9V will be around value 150.

**GAL chip operations:**
* With the GAL chip inserted and power button pressed (or in ON position) check the chip identification by running the following command:
  <pre>
  ./afterburner i -t [GAL_type]
  </pre>

  If you get some meaningful GAL chip identification like:
  <pre>
  PES info: Atmel ATF16V8B  VPP=10.0 Timing: prog=10 erase=25
  </pre>
  then all should be well and you can try to  erase the chip and then program it to contain your .jed file.

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
  be erased - even if it is a brand new chip that has not been used 
  before. Use the following command:
  <pre>
  ./afterburner e -t [GAL type]
  </pre>

* Program and verify the GAL chip via the following command:
  <pre>
  ./afterburner wv -t [GAL type] -f my_new_gal.jed
  </pre>

* If you are not sure which GAL type strings are accepted by Afterburner, simply set a wrong type and it will print the list of supported types: 
  <pre>
  ./afterburner wv -t WHICH
  </pre>

How aferburner works:
---------------------
- PC code reads and parses .jed files, then uploads the data to Arduino via serial port. By default /dev/ttyUSB0 is used, but that can be changed to any other serial port device by passing the following option to afterburner:
  <pre>
  -d /my/serial/device
  </pre>

- PC code of afterburner communicates with Arduino UNO's afterburner
  sketch by a trivial text based protocol to run certain commands (like erase, read, write, upload data etc.). If you are curious, you can also connect directly to Arduino UNO via serial terminal and issue some basic commands manually.

- Arduino UNO's afterburner sketch does 2 things: 
  * parses commands and data sent from the PC afterburner app
  * toggles the GPIO pins and drives programming of the GAL contents

- more information about GAL chips and their programming can be found here:

  http://www.bhabbott.net.nz/atfblast.html
  
  http://www.armory.com/%7Erstevew/Public/Pgmrs/GAL/_ClikMe1st.htm 


PCB:
---------------
The new design no longer has an etched PCB design available. The most convenient way to get the PCB is to order it online on jlcpcb.com, pcbway.com, allpcb.com or other online services. Use the zip archive stored in the gerbers directory and upload it to the manufacturer's site of your choice.
  Upload the afterburner_fab_3_0.zip and set the following parameters (if required). 
  
  * Dimensions are 85x54 mm
  * 2 layer board
  * PCB Thickness: 1.6, or 1.2
  * Copper Weight: 1
  * The rest of the options can stay default or choose whatever you fancy (colors, finish etc.)
  
Soldering steps:
----------------
  * check which type of components you have, you can mix and match SMT and through hole components as most of the footprints are doubled to accommodate different parts.
  * **Important:** C5, C6 and C7 are VPP decoupling capacitors and must be rated to **at least 25V!** You can use 50V rated caps, but do not use 16V or lower ratings.
  * Even though C5 (10uF, 25V) offers a SMT footprint, I used a through hole part because it reduces the VPP swings better than my SMT cap.
  * start with the smallest parts, solder the resistors and small capacitors.
  * solder the two ICS: U1 (digital pot) and U2 (shift register)
  * solder the LED, the switch and the big capacitors

  **When using MT3608 module**
  * do **not** solder any components in voltage booster area
  * **special step** solder a thin wire between the MT3608 module and the PCB hole marked as POT. See the image bellow.
  * **Important** after you solder the wire measure the resistance between the wire's connection on Afterburner PCB and ground (use TP GND hole) - see red dots
     on the picture beelow.  It must be around 12.5 kilo Ohm. If it is very low value then you made a short while soldering the wire. Fix/remove the short
     or else the MT3608 will be damaged when it is turned on on.
       ![POT wire image.](https://github.com/ole00/afterburner/raw/master/img/mt3608_wire.jpg "POT wire")
  * solder the MT3608 module - the POT connection wire must be already soldered!

  **When using on-board voltage booster**
  * solder the parts in the voltage booster area. Start with the U3 IC - the SOT-23-6 package - so that you have plenty
    of room for soldering the small IC. Use a flux to ensure the solder melts nicely on the pads.
  * solder the inductor L1 and diode D2. Ensure D2 polarity is correct - the stripe is on the left side (see photos if unsure).
  * solder the C10 cap and the resistors. There is no need to solder R10, R11, R12 if you are using the alternative booster IC
    with SOT-23-5 package (without OVP)
  * no extra wire is required (the wire is only required when using MT3608 module)

  **After soldering the voltage booster** 
  * calibrate the board. See calibration steps in the Setup section.
  * solder the ZIF socket


MT3608 modules:
---------------
  * On PCB version 3.2 (or higher) the MT3608 can be replaced by discrete parts soldered on the board. Therefore, if you want to avoid possible issues with
    MT3608 modules, solder the discreate parts instead of the module. If you are not comfortable soldering SOT-23-6 SMT IC package, then use the MT3608 module.
  * There is a report some of the MT3608 modules / clones are not compatible with Afterburner. Thanks @meup for the information.
  * The incompatible MT3608 'clone' causes calibration issue and basically breaks the variable voltage functionality. This can be fixed by replacing the 2k2 resistor located on the module by a 15k resistor.
  * Bellow is the image of the old (compatible without a mod) and new (require the resistor mod) MT3608 modules. If you happen to have the incompatible module here are the steps to replace the resistor:
      - heat up a soldering iron to ~350 deg C and put a medium sized solder blob on its tip
      - touch the tip of the iron in the middle of the resistor for a second
      - gently press on the resistor and slide it off the PCB.
      - the resistor footprint allows to solder new SMT or through hole resistor
  
  ![mt3608_modules](https://github.com/ole00/afterburner/raw/master/img/mt3608_modules.jpg "mt3608_modules")


3D printed case:
----------------
There is a model of a case/enclosure designed for 3D printers in the 'case' directory. The following three printed parts/pieces are required for a complete enclosure: top, bottom and switch button parts. 
There are 2 sets of STL files provided: One for Arduino UNO R3 with USB-B connector, and the other for Arduino UNO R4 with USB-C connector. The STL file names of these sets are clearly distinguished.
The enclosure design has more options available - like an option to have VPP measurement holes, option to disable Afterburner text and others. These can be enabled in design file (atf3x.scad) presets
when it's opened in OpenScad editor. 

To export STL files from OpenScad:

* ensure you have the right part (and only the right part) enabled in the options (top, bottom or button switch).
* press F6 and wait for the model to be calculated.
* press F7 to export the STL file of the part.

When slicing the models, ensure the model supports are enabled, especially for the top part requires that.

The top and bottom parts are joined together with M3 screws and M3 threaded heat inserts.

Bellow are 2 variants of the enclosure, the one on the left has VPP measurement holes. Ususally the VPP calibration/measurement holes
are not required as the VPP calibration can be done before the PCB is put into the case.

  ![mt3608_modules](https://github.com/ole00/afterburner/raw/master/img/3d_printed_cases.jpg "3D printed cases")

If the top and bottom printed parts can't be easily joined and something seems to be blocking please check:
- the LED is precisely positioned to fit
- the through hole ceramic capacitors around the ZIF socket are not too close to the ZIF socket. Bend them a bit if they are too close.
- the through hole Mosfet is not too close to the ZIF socket
- check that other components, especailly through hole are not sticking high above the Afterburner's PCB. The case was designed
  and tested with through hole components, but if the compoenent leads are too long the component might block the case.
- ensure the button's cut/flat part is facing the ZIF socket, the round bit is facing away from the ZIF socket.


Troubleshooting:
----------------
- it does not work!

  * double check the solder joints on the PCB, ensure they are soldered. Especially SMT soldering when done manually can accidentally miss some of the pads. 
  
  * ensure the GAL chip is inserted to the IC socket the right way (check 
  the pin 1 location). Note that the 16V8 GALs have to be inserted close to the ZIF lever - see the title image above.
  
  * ensure the VPP is set correctly on the MT3608 module. Ensure you've gone through all the calibration steps (commands: 's' then 'b' and 'm') and calibration is correct. See the Setup section.

- what is the Push switch used for? When do I use it?
  * Normally, the button should be in the Off position (LED is not lit). Also when inserting
    the GAL chip or when removing the GAL chip from the ZIF socket the switch should be Off.
  * When using Afterburner's PC app with commands to Identify (i), Read or Verify (r, v) or 
    Write (w) the switch has to be On (LED is lit).
  * So in general: insert the GAL when the switch is Off, then turn the switch On, run the 
    PC app command. When finished turn the switch Off, remove the GAL chip from the socket.

- afterburner reports it can not connect to /dev/ttyUSB0, permission denied

  * ensure your user is member of the of the dialup group or alternatively run
  afterburner  as superuser (use: sudo ./afterburner ...)

  * ensure no other program on your PC uses that serial port. Close putty, minicom or other
  terminals you may be running.

- afterburner fails to connect to /dev/ttyUSB0

  * ensure your PC is actually connected to Arduino UNO, check a serial device
  exists:
  <pre>
  ls -alF /dev/ttyUSB*
  </pre>

- I can't program GAL18V10
  * You'll need PCB version 3.1. If you have PCB version 3.0 you can mod it.
    See ![here](https://github.com/ole00/afterburner/pull/36) for more information about the mod.
  * Some GAL18V10B from Aliexpress do not work with Afterburner (fakes? damaged?).
    My GAL18V10B-15LP from Aliexpress do not work. However, GAL18V10B-**20LP** do work OK (also from Aliexpress).

- I want to program ATF16V8C, but it is not listed as supported by the PC app.
  * use parameter '-t ATF16V8B'. Afterburner finds out it is a C version.

- I forgot to solder the POT wire to the MT3608 module. The MT3608 is already soldered and I can't reach underneath the module to solder the wire.

  * you can either desolder the module by using soldering wick (to remove all solder on the connection pins on the MT3608 module). Then use a low temperature melting solder (like Quick Chip or similar) on the connection joints to loosen up the module. Clean the residues of the low melting solder with soldering wick. Then solder the POT wire and solder the chip back on the PCB. The drawback of this method is that if you use excessive heat during desoldering you can damage the MT3608 module (I've done that). If the module is damaged, it will produce a magic smoke next time the board is turned on. If that happens, desolder the module, use a new module (don't forget to solder the POT wire) and solder it on the board.

  * Another option is to connect the POT wire directly to the MT3608 IC's Feedback (FB) pin 3. This is quite delicate as the IC pins are quite small. Before connecting the power, **ensure the pin 2 and pin 3 are not bridged!**

  <pre>
  |  Blue Potentiometer     |
  |_________________________|
  |   ______________
  |  |              |
  |  | SMT Capacit. |
  |  |______________|
  |
  |   3  2  1
  |-> |  |  |         MT3608 MODULE
  |  +-------+
  |  |       |
  |  |4  5   |
  |  +-------+
  |   |  |  |
  |
  |<-- left long edge of the module
  </pre> 

  * The small arrow (->) in the above diagram marks the pin where the wire has to be soldered.

- my MT3608 module does not seem to work - turning the pot does nothing

  * The pot needs to be turned at least 10 revolutions counter-clockwise
  to do anything useful. Keep turning.

- where to get the MT3608 module ?

  * usual places: ebay, aliexpress
  * **IMPORTANT - check the module compatibility - there are some incompatible modules (see above)**

- how to run afterburner on Win32, Win64 or MacOSX ?
  * the same way as on Linux: compile the source code
  * OR use pre-compiled afterburner binaries located in 'releases' directory
  * then run afterburner in terminal (use 'cmd' on WinXX) as described above
  * ensure your serial device name is passed via '-d' option. For example -d COM5 on WinXX

- I have the older Afterburner PCB design, can I use the new PC software and Arduino sketch?
  * Yes, both programs are compatible with the old Afterburner boards (1.X and 2.X).

- how do I program ATF150X ICs? They do not fit into the ZIF Socket.
  There are 2 options. Either use PLCC44 IC package along with the ZIF socket adapter (see the gerber and
  pcb directory). Or you can program the ATF150X on your target board when using JTAG interface.
  See this schematics for information about JTAPG pins on ATF150X ICs:
  http://matthieu.benoit.free.fr/all03/adp/HiLo_ADP-ATF1504.PDF
  The OGI pin in the schematic is VPP (or EDIT) pin on afterburner. See afterburner schematics
  how to connect JTAG pins from the ZIF socket into the JTAG pins on your board.

- what are the .jed files and how to produce them
  
  * Use WinCUPL software from Atmel. Works under Wine as well.
  
  https://www.microchip.com/design-centers/fpgas-and-plds/splds-cplds/pld-design-resources
  
  * WinCUPL User's manual: http://ww1.microchip.com/downloads/en/DeviceDoc/doc0737.pdf
  
  * Try GAL Asm to produce .jed files - see link bellow.

- can I use .jed files with ATF150X IC?
  * Not directly. You have to convert the .jed file into .xsvf format. Use the python tools located in the utils/jtag
    subdirectory to do that. See readme.txt in that directory for more info. Once you convert the .jed to .xsvf
    you can use it with afterburner like that:
    <pre>
    ./afterburner -t ATF1502AS -f mydesign.xsvf ew
    </pre>
    which will erase the chip and then write your design into the IC.
    See discussion #64 (ATF1502AS(L) and ATF1504AS(L) support) for more inofrmation.

  
Other GAL related links:
------------------------
- GAL chip info: https://k1.spdns.de/Develop/Projects/GalAsm/info/galer/gal16_20v8.html

- GAL chip programming protocol info: https://k1.spdns.de/Develop/Projects/GalAsm/info/galer/proggal.html

- GALmate: another open source GAL programmer: https://www.ythiee.com/2021/06/06/galmate-hardware/
  
- JDEC file standard 3A: https://k1.spdns.de/Develop/Projects/GalAsm/info/JEDEC%20File%20Standard%203A.txt
  
- GAL Asm : https://github.com/dwery/galasm

- GAL Asm online compiler: https://rhgndf.github.io/galasm-web/

- PLD and GAL info: https://github.com/peterzieba/5Vpld

- Fusemap info:
    * https://blog.frankdecaire.com/2017/01/22/generic-array-logic-devices/
    * https://blog.frankdecaire.com/2017/02/25/programming-the-gal22v10/
 
- CUPL Reference: https://web.archive.org/web/20220126145737/https://ee.sharif.edu/~logic_circuits_t/readings/CUPL_Reference.pdf
