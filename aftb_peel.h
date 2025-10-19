#include "Arduino.h"
#pragma once

/* 
PEEL device functions for Afterburner GAL project.
==================================================

supported ICs: PEEL18CV8


PEEL devices require 'high' voltage ( > 12V) on 4 distinct pins during programming:
- PVE pin : 12.5 V
- PVP pin : 15.5 V
- ALE/ERA pin : 15.5 V when erasing the chip
- SEC : 15.5 V when securing the chip. That's not currently supported to keep the adapter simple

Afterburner provides only one pin with variable 'high' voltage : VPP. To program PEEL devices
the 3 'high' voltages are derived from VPP and routed via MOSFETs located on the adapter
board to their respective pins.

PEEL devices use 8 bit bi-directional parallel bus for data transfer, compared to 1 bit bus
of GAL devices. Also, there are additional 6 pins on the PEEL device dedicated for 
setting various mode/status during programming. That depletes Afterburner's available pin 
count, therefore the slow shift register pins (on Aferburner's PCB 3.2 and higher) are used 
for setting the status mode/straps on a PEEL device and for controlling some of the MOSFETs.

The pin mapping is as follows:
- IADx  are input/output 8 bit data bus pins:
  Arduino pins D5 - D12 (ZIF8, ZIF9,ZIF10, ZIF11, ZIF16, ZIF15, ZIF14, ZIF13)

- ALE/ERA: Arduino pin D4 (ZIF22)
- SEC/OD : Arduino pin D2 (ZIF3)
- CTL_PVP: Arduino pin D3 (ZIF23)
  
- TGA pins:  SHR Q1-Q4 (ZIF4, ZIF7, ZIF6, ZIF5)
- MODE pins: SHR Q5-Q6 (ZIF21, ZIF20)

- VPP pin (ZIF2)
- VCC pin (ZIF24)
- GND pin (ZIF12)

- auxiliary pin PVE_VH1: SHR Q7 (ZIF19)
- auxiliary pin ERA_VH2: SHR Q8 (ZIF18)

*/

#define PEEL_DEBUG_BIT_ERRORS 0

#define PIN_SEC_OD        2
#define PIN_CTL_PVP       3
#define PIN_ALE_ERA       4

// when ERA_VH2 is set then the ALE/ERA pin follows PVP (up to 15.5V). Used for Erase operation.
#define ERA_VH2  0b10000000

// When ERA_OFF is set, then ALE/ERA follows output of PIN_ALE_ERA pin (0-4.7V). Used for Read and Write operations.
#define ERA_OFF 0b00000000

#define PVE_VH1 0b00000000

#define PVE_OFF 0b01000000

#define MODE_A_H 0b00010000
#define MODE_A_L 0b00000000

#define MODE_B_H 0b00100000
#define MODE_B_L 0b00000000


#define OP_ERASE 1
#define OP_WRITE_ALL 0

// Note that when using PIN_ALE_ERA High, ensure that ERA_OFF is applied - ie. ALE/ERA does not follow PVP.


static const uint8_t inputLinesPEEL[] PROGMEM =
{
    1,3,  2,4,  5,7,  0x21,0x23,  9,0xB,  0x1D,0x1F,  0xD,0xF,  0x19,0x1B,  0x11,0x13, 0x15,0x17, 0x12,0x14,
    0x18,0x16,  0xE,0x10,  0x1C,0x1A,  0xA,0xC,  0x20,0x1E,  6,8,  0x24,0x22
};

static const uint8_t dataBusPinsPEEL[] PROGMEM =
{
    PIN_ZIF8, PIN_ZIF9, PIN_ZIF10, PIN_ZIF11, PIN_ZIF16, PIN_ZIF15, PIN_ZIF14, PIN_ZIF13
};

static void setDataDirectionPEEL(uint8_t setDirection)
{
    uint8_t i = 0;
    while (i < 8) {
        pinMode(pgm_read_byte(&dataBusPinsPEEL[i]), setDirection);
        i++;
    }
}

// set a value on PEEL data bus 
static void setDataPEEL(uint8_t d)
{
    uint8_t i = 0;
    uint8_t mask = 1;
    while(i < 8) {
        digitalWrite(pgm_read_byte(&dataBusPinsPEEL[i]),  (d & mask) ? HIGH : LOW );
        mask <<= 1;
        i++;
    }
}

// read PEEL data bus
static uint8_t getDataPEEL()
{
    uint8_t d = 0;
    uint8_t i = 0;
    while(i < 8) {
        d <<= 1;
        d |=  digitalRead(pgm_read_byte(&dataBusPinsPEEL[7-i])) ? 1 : 0;
        i++;
    }
    return d;
}

// Ramp up from 0 - 15.5V in 775 uSec or slower
// The function bellow ramps up in about 1000 uSec.
static void rampUpVppPEEL()
{
    uint8_t i = 0;
    while (i < 20) {
        i++;
        digitalWrite(PIN_CTL_PVP, HIGH);
        delayMicroseconds(1);
        digitalWrite(PIN_CTL_PVP, LOW);
        delayMicroseconds(90 - (i << 2));
        //delayMicroseconds(100 - (i << 2));
    }
    digitalWrite(PIN_CTL_PVP, HIGH);
}

static void operationSetupPEEL(uint8_t shreg)
{
    //step 0 - IAD(0-7): Set to high impedance
    setupGpios(INPUT); 

    //ALE/ERA Set to V_il 
    pinMode(PIN_ALE_ERA, OUTPUT);
    digitalWrite(PIN_ALE_ERA, LOW);

    //SEC/OD Set to Vil
    pinMode(PIN_SEC_OD, OUTPUT);
    digitalWrite(PIN_SEC_OD, LOW);

    // Setup PVP Control
    pinMode(PIN_CTL_PVP, OUTPUT);
    // disable PVP, external pull-down resitor keeps PVP low
    digitalWrite(PIN_CTL_PVP, LOW);

    // PVE LOW
    setShiftReg(shreg);
    digitalWrite(PIN_SHR_EN, LOW); //enable output of the shift register

    delay(1);
    setVPP(ERASEALL, 4); // VPP to 16V
}

// erases fuse map in the PEEL device (all fuses to 1)
// program-all fuse map in the PEEL device (all fuses to 0)
static void eraseProgramAllPEEL(uint8_t pulseLen, uint8_t erase)
{
    uint8_t shreg = MODE_A_L | MODE_B_L | PVE_OFF | ERA_OFF;

    // write-all needs MODE_B High
    if (!erase) {
        shreg |= MODE_B_H;
    }

    operationSetupPEEL(shreg);

    // step 1 : PVE to Vhh1
    shreg &= ~(PVE_OFF);
    setShiftReg(shreg);


    // MOD -A,B already set to 0
    // TGA - already set to 0

    // Step 5 - Set address based on the operation
    setDataDirectionPEEL(OUTPUT);
    setDataPEEL(erase ? 0 : 0xFF);

    delayMicroseconds(2); //T-AL

    // Step 6 - ALE Era follows PVP
    shreg |= ERA_VH2;
    setShiftReg(shreg);

    
    //ALE Era follows PVP when erasing
    if (erase) {
        shreg |= ERA_VH2;
        setShiftReg(shreg);
    }

    // Step 7
    rampUpVppPEEL();

    delay(pulseLen); // 100 ms for preconditioning

    digitalWrite(PIN_CTL_PVP, LOW); // The bleed resistor pulls PVP to 0V
    delay(20); // 20 ms to go to 3V,  40 ms to fully go to 0V

    if (!erase) {
        delayMicroseconds(2); //T-FP

        //Step 7 IAD to High Z
        setDataPEEL(0);
        setDataDirectionPEEL(OUTPUT);

        delayMicroseconds(2); //T-PH
    }

    // Step 8 - PVE to Low
    setVPP(0, 0); // VPP low to 5 V, no settle time
    // step 14 : PVE to Low
    shreg |= PVE_OFF;
    setShiftReg(shreg);
    delay(50); // VPP ramp down
    setupGpios(INPUT); 
}

static uint8_t getInputLineAddressPEEL(uint16_t index, uint8_t* tga)
{
    // Product terms
    if (index < 288) { // fuse 2304
        *tga = 8 - (index / 36);
        return pgm_read_byte(&inputLinesPEEL[index % 36]);
    }
    // OE terms
    if (index < 324) { // fuse 2592
        *tga = 9;
        return  pgm_read_byte(&inputLinesPEEL[index - 288]);
    }
    // SP & AC terms
    if (index < 360) {// 333 (fuse 2664) + 27: 
        *tga = 10;
        return  pgm_read_byte(&inputLinesPEEL[index - 324]);
    } else {
    // Macrocell config - up to byte index 367 (including)
        *tga = 8 - (index - 360);
        return 0x25;
    }

    return 0;
}

// reads a fuse map and extracts an 8 bit value based on index i
static uint8_t getFuseBytePEEL(uint16_t i)
{
    uint16_t fuseIndex;
    uint8_t d;

    if (i < 324) { // Product terms or OE Terms - 8 bits are used
        uint8_t  mc;
        if (i < 288) {
            fuseIndex = (i / 36);
            fuseIndex *= 288;
            fuseIndex += (i % 36);
        } else {
            fuseIndex = i - 288;
            fuseIndex += 2304;
        }
        d = 0;
        // reverse the order of bits: first bit goes to bit position 7
        for (mc = 0; mc < 8; mc++) {
            d <<= 1;
            if ( getFuseBit(fuseIndex)) {
                d |= 1;
            }
            fuseIndex += 36;
        }
    } else
    if (i < 360) { //SP & AC terms - only 2 lower bits are used
        fuseIndex = i - 324;
        //SP starts at fuse 2592
        //AC starts at fuse 2628
        d = getFuseBit(2592 + fuseIndex); // SP
        d <<= 1;
        d |= getFuseBit(2628 + fuseIndex); // AC
    } else { // Macrocell configuration: 4 bits are used
        uint8_t t;
        t = fusemap[333 + ((i - 360) >> 1)];
        t >>= (((i-360) & 0b1) << 2);
        // the 4 configuration bits are dispered in 0bDxCxBxAx fassion within the byte
        d = ((t & 0b1000) << 4) | ((t & 0b100) << 3) | ((t & 0b10) << 2 ) | ((t & 0b1) << 1 );
    }
    return d;
}

// writes a byte (d) into fuse map based on index (i)
static void setFuseBytePEEL(uint16_t i, uint8_t d)
{
    uint16_t fuseIndex;
    if (i < 324) { // OE terms
        uint16_t  mc;
        if (i < 288) {
            fuseIndex = (i / 36);
            fuseIndex *= 288;
            fuseIndex += (i % 36);
        } else {
            fuseIndex = i - 288;
            fuseIndex += 2304;
        }
        // reverse the order of bits: first bit goes to bit position 7
        for (mc = 0; mc < 8; mc++) {
            setFuseBitVal(fuseIndex, d & 0b10000000);
            fuseIndex += 36;
            d <<= 1;
        }
        return;
    }
    if (i < 360) {  //SP & AC terms - only 2 lower bits are used
        fuseIndex = i - 324;
        //SP starts at fuse 2592
        //AC starts at fuse 2628
        setFuseBitVal(2628 + fuseIndex, d & 1); // AC
        d >>= 1;
        setFuseBitVal(2592 + fuseIndex, d & 1); // SP

        return;
    }
    { // // Macrocell configuration: 4 bits are used
        uint8_t t;
         // the 4 configuration bits are dispered in 0bDxCxBxAx fassion within the byte
        t = ((d & 0b10) >> 1) | ((d & 0b1000) >> 2) | ((d & 0b100000) >> 3) | ((d & 0b10000000) >> 4);
        t <<=  (((i-360) & 0b1) << 2);
        fusemap[333 + ((i - 360) >> 1)] |= t;
    }
}

// reading or verification of a fuse map
static uint16_t readVerifyFuseMapPEEL(uint8_t verify)
{
    uint16_t bitErrors = 0; 
    uint8_t d;
    uint16_t i;
    uint8_t shreg = MODE_A_H | MODE_B_L | PVE_OFF | ERA_OFF;

    operationSetupPEEL(shreg);

    // step 1 : PVE to Vhh1
    shreg &= ~(PVE_OFF);
    setShiftReg(shreg);

    // step 2 : MODE A and B - already set via shift register
    // step 3 : ALE/ERA Set to V_il  - already set
    delayMicroseconds(40); //T-PS

    for (i = 0; i < 337 + 31; i++) {
        uint8_t tga;
        uint8_t inputLineAddress;

        inputLineAddress = getInputLineAddressPEEL(i, &tga);
        // step 4 : set TGA 0-3 
        shreg &= ~(0b00001111);
        shreg |= tga;
        setShiftReg(shreg);

        // step 5 :  set input line address
        setDataDirectionPEEL(OUTPUT);
        setDataPEEL(inputLineAddress);

        delayMicroseconds(150); //T-AL

        // step 6 : set SEC/OD to Vh
        digitalWrite(PIN_SEC_OD, HIGH);

        // step 7 : set ALE/ERA to Vh (Latch address)
        digitalWrite(PIN_ALE_ERA, HIGH);

        delayMicroseconds(2); //T-LA

        // step 8: IAD high impedance
        setDataDirectionPEEL(INPUT);

        delayMicroseconds(2); //T-AS

        // Step 9: Set PVP to Vhh2 - slow ramp/up
        rampUpVppPEEL();

        delayMicroseconds(2); //T-PL

        // Step 10: ALE/ERA low
        digitalWrite(PIN_ALE_ERA, LOW);

        //  Step 11: SEC/OD Low
        digitalWrite(PIN_SEC_OD, LOW);

        delayMicroseconds(2); //T-DV

        // Step 12 : read fuse data
        setDataDirectionPEEL(INPUT_PULLUP);
        delayMicroseconds(150);
        d = getDataPEEL();

        delayMicroseconds(8); //T-VP

        // Step 13 : Set PVP Low
        digitalWrite(PIN_CTL_PVP, LOW); // The bleed resistor pulls PVP to 0V

        // TODO - optimise for speed, delay only 4 ms on Adapter Rev. 2. Speed-up by 7.5 seconds
        delay(30); // 20 ms to go to 3V (did not work well),  40 ms to fully go to 0V

        if (verify) {
#if PEEL_DEBUG_BIT_ERRORS
            uint16_t e = bitErrors;
#endif
            uint8_t x = getFuseBytePEEL(i);

            // specific offsets require certain bits to be erased from 'd'
            if (i >=324) {
                // SP& AC terms - only 2 lower bits are used
                if (i < 360) {
                    d &= 0b11;
                } else
                // Config bits - only 4 dispersed bits are used
                {
                    d &= 0b10101010;
                }
            }

            if ((d & 0b1) != (x & 0b1)) bitErrors++;
            if ((d & 0b10) != (x & 0b10)) bitErrors++;
            if ((d & 0b100) != (x & 0b100)) bitErrors++;
            if ((d & 0b1000) != (x & 0b1000)) bitErrors++;
            if ((d & 0b10000) != (x & 0b10000)) bitErrors++;
            if ((d & 0b100000) != (x & 0b100000)) bitErrors++;
            if ((d & 0b1000000) != (x & 0b1000000)) bitErrors++;
            if ((d & 0b10000000) != (x & 0b10000000)) bitErrors++;
#if PEEL_DEBUG_BIT_ERRORS
            if (bitErrors != e) {
                Serial.print(F("index="));
                Serial.println(i, DEC);
                Serial.print(F("x="));
                Serial.println(x, HEX);
                Serial.print(F("d="));
                Serial.println(d, HEX);
            }
#endif
        } else {
            setFuseBytePEEL(i, d);
        }
        delayMicroseconds(1); //T-MH
    }
    setVPP(0, 0); // VPP low to 5 V, no settle time
    // step 17 : PVE to Low
    shreg |= PVE_OFF;
    setShiftReg(shreg);
    delay(50); // VPP ramp down
    setupGpios(INPUT); 
    return bitErrors;
}

// writing fuse map
static void writeFuseMapPEEL(void)
{
    uint16_t i;
    uint8_t d;
    uint8_t shreg;

    shreg = MODE_A_H | MODE_B_H | PVE_OFF | ERA_OFF;

    operationSetupPEEL(shreg);

    // step 1 : PVE to Vhh1
    shreg &= ~(PVE_OFF);
    setShiftReg(shreg);

    // step 2 : MODE A and B - already set via shift register
    // step 3 : ALE/ERA Set to V_il  - already set
    delayMicroseconds(40); //T-PS //40

   // +31 (27+4) are virtual bytes for A&C and Config fuses that are not entered in 8bits, but in 2 bits and 4 bits respectively
    for (i = 0; i < 337+31; i++) {  
        uint8_t tga;
        uint8_t inputLineAddress;

        inputLineAddress = getInputLineAddressPEEL(i, &tga);

        // step 4 : set TGA 0-3 
        shreg &= ~(0b00001111);
        shreg |= tga;
        setShiftReg(shreg);

        // step 5 :  set input line address
        setDataDirectionPEEL(OUTPUT);
        setDataPEEL(inputLineAddress);

        delayMicroseconds(2); //T-AL

        d = getFuseBytePEEL(i);

        delayMicroseconds(150); //T-AL

        // step 6 : set ALE/ERA to Vh (Latch address)
        digitalWrite(PIN_ALE_ERA, HIGH);

        delayMicroseconds(20); //T-LA

        // step 7: set fusemap to IAD
        setDataPEEL(~d); // when writing, the fuse bits must be inverted
  
        delayMicroseconds(150); //T-AS

        // Step 8: Set PVP to Vhh2 - slow ramp/up
        rampUpVppPEEL();

        delayMicroseconds(10); //T-PL
        // Step 9: ALE/ERA low
        digitalWrite(PIN_ALE_ERA, LOW);

        delay(11); //T-WP

        // Step 10 : Set PVP Low
        digitalWrite(PIN_CTL_PVP, LOW); // The bleed resistor pulls PVP to 0V

        // TODO - optimise for speed, delay only 4 ms on Adapter Rev. 2. Speed-up by 7.5 seconds
        delay(30); // 20 ms to go to 3V,  40 ms to fully go to 0V
    }
    setVPP(0, 0); // VPP low to 5 V, no settle time
    // step 14 : PVE to Low
    shreg |= PVE_OFF;
    setShiftReg(shreg);
    delay(50); // VPP ramp down
    setupGpios(INPUT); 
}

// precondition and erase PEEL device
static void erasePEEL(void)
{
    eraseProgramAllPEEL(100, OP_ERASE); // preconditioning fuse bits to 1
    eraseProgramAllPEEL(100, OP_WRITE_ALL); //sets all fuse bits to 0
    eraseProgramAllPEEL( 10, OP_ERASE); //sets all fuse bits to 1
}

static void  printMeasurePEEL(uint8_t t)
{
    Serial.print(F("TP2=15.5V TP3="));
    Serial.print(t & 0b01 ? F(" 0.0V") : F("12.5V"));
    Serial.print(F(" TP4="));
    Serial.print(t & 0b10 ? F(" 0.0V") : F("15.5V"));
    Serial.print(F(" TP5="));
    Serial.println(t == 0 ? F("15.3V") : t == 1 ? F(" 0.0V") : F(" 4.7V"));

}

// test & measure programming voltages on PEEL adapter
static void  measureVoltagesPEEL(void)
{
    uint8_t shreg = MODE_A_L | MODE_B_L | PVE_VH1 | ERA_VH2;

    setGalDefaults();

    Serial.println(F("Check PEEL test points, power switch ON, tolerance +/- 0.3V"));

    // VPP 15.5V   PVE=12.5  PVP=15.5   ALE/ERA=15.5V
    digitalWrite(PIN_ALE_ERA, HIGH);

    operationSetupPEEL(shreg);
    rampUpVppPEEL();
    printMeasurePEEL(0);
    delay(10000);

    // VPP 15.5V   PVE=0V  PVP=15.5   ALE/ERA=0V
    shreg |= PVE_OFF;
    shreg &= ~(ERA_VH2);
    setShiftReg(shreg);
    printMeasurePEEL(1);
    delay(10000);

    // VPP 15.5V  PVE=12.5V  PVP=0V   ALE/ERA=4.5V
    shreg &= ~(PVE_OFF);
    setShiftReg(shreg);
    digitalWrite(PIN_ALE_ERA, HIGH);
    digitalWrite(PIN_CTL_PVP, LOW); // The bleed resistor pulls PVP to 0V
    printMeasurePEEL(2);
    delay(10000);


    // turn all off
    digitalWrite(PIN_ALE_ERA, LOW);
    setVPP(0, 0); // VPP low to 5 V, no settle time
    // step 14 : PVE to Low
    shreg |= PVE_OFF;
    setShiftReg(shreg);
    delay(50); // VPP ramp down
    setupGpios(INPUT);

    Serial.println(F("OK Finished"));

}
