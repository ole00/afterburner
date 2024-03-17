/*
 * Variable voltage functions for Afterburner GAL project.
 * 
 * 2024-02-02 Minor changes in varVppInit()
 */
#ifndef __AFTB_VPP_H__
#define __AFTB_VPP_H__

#include <EEPROM.h>

// ensure mcp4131 pot uses the right pins
#define POT_CS   A3
#define POT_CLK  A4
#define POT_DAT  A5
#define VPP      A0

#include "aftb_mcp4131.h"
#ifndef FAIL
#define FAIL 0
#define OK 1
#endif

#define ABS(X) ((X) < 0 ? -(X) : (X));
#define VPP_5V0   0xFF
#define VPP_9V0   0
#define VPP_9V5   1
#define VPP_10V0  2
#define VPP_10V5  3
#define VPP_11V0  4
#define VPP_11V5  5
#define VPP_12V0  6
#define VPP_12V5  7
#define VPP_13V0  8
#define VPP_13V5  9
#define VPP_14V0  10
#define VPP_14V5  11
#define VPP_15V0  12
#define VPP_15V5  13
#define VPP_16V0  14
#define VPP_16V5  15

#define MAX_WIPER 16

#define VPP_VERBOSE 0

#ifdef EXTERNAL
#define ANALOG_REF_EXTERNAL EXTERNAL
#else
#define ANALOG_REF_EXTERNAL AR_EXTERNAL
#endif

//UNO R4 Minima or Wifi (Aref internally pulled down by 130kOhm, AVR Uno R3 pulled down by 32kOhm)
#ifdef _RENESAS_RA_
#define AREF_IS_3V2
#endif

//pot wiper indices for the voltages 
uint8_t vppWiper[MAX_WIPER] = {0};

// VPP must ramp-up to prevent voltage spikes and possibly resetting arduino
// These values are for mcp4151  (for mcp4131 they are divided by 2)
#define varVppSetMax() varVppSetVppIndex(0x80); \
                        varVppSetVppIndex(0xE0); \
                        varVppSetVppIndex(0xF4); \
                        varVppSetVppIndex(0xFA); \
                        varVppSetVppIndex(0xFF); 


#define varVppSetMin() varVppSetVppIndex(0x0);

uint8_t wiperStat = 0; //enabled / disabled
int8_t calOffset = 0; // VPP calibration offset: value 10 is 0.1V, value -10 is  -0.1V

static void varVppReadCalib(void) {
    uint8_t i;
    //calibration not found
    if (EEPROM.read(0) != 0xAF || EEPROM.read(1) != 0xCA) {
        vppWiper[0] = 0;
        Serial.println(F("No calibration data in EEPROM"));
        return;
    }
    calOffset = (int8_t) EEPROM.read(2);
    for (i = 0; i < MAX_WIPER; i++) {
        vppWiper[i] = EEPROM.read(i + 3);
#if 0        
        Serial.print(F("Calib "));
        Serial.print(i);
        Serial.print(F(":"));
        Serial.println(vppWiper[i]);
#endif
    }
}

// internal use only - set the wiper value on the digital pot
static void varVppSetVppIndex(uint8_t value) {
    uint8_t i;

#if VPP_VERBOSE
    Serial.print(F("varSetVppIndex "));
    Serial.println(value);
#endif
    mcp4131_write(ADDR_WIPER, value);
#if VPP_PARANOID
    i = mcp4131_read(ADDR_WIPER);
    if (i != value) {
        Serial.print(F("Error writing POT value. Expected:"));
        Serial.print(value);
        Serial.print(F(" Actual:"));
        Serial.println(i);
    }
#endif
    if (value == 0) {
        mcp4131_disableWiper();
        wiperStat = 0;
    } else if (wiperStat == 0) {
        mcp4131_enableWiper();
        wiperStat = 1;
    }
}

//use by the app code - set the variable voltage
static void varVppSet(uint8_t value) {
    uint8_t v;
    int8_t inc;
    int8_t incMin;
    if (value == VPP_5V0 || value >= MAX_WIPER) {
        varVppSetVppIndex(0);
        return;
    }
#if VPP_VERBOSE
    Serial.print(F("varSetVpp "));
    Serial.print(value);
    Serial.print(F(":"));
    Serial.println(vppWiper[value]);
#endif
    //ramp up to prevent massive voltage overshoots
    v = vppWiper[value] / 2;
    v -= 2;
    inc = 16;
    incMin = 2;
    if (value > VPP_13V0) {
        incMin = 1;
    }
    while (v < vppWiper[value]) {
        varVppSetVppIndex(v);
        v+= inc + (inc / 2);
        inc -= inc / 2;
        if (inc < incMin) {
            inc = incMin;
        }
    }
    varVppSetVppIndex(vppWiper[value]);
}

// UNO R4/Minima - Renesas IC (significant ADC gain errors measured)
#ifdef AREF_IS_3V2
#define SAMPLE_CNT 16
#define SAMPLE_DIVIDER 8
#define SAMPLE_MULTIPLIER 25
// SAMPLE_SHIFT moves the ADC gain error up/down
#define SAMPLE_SHIFT -45;

//AVR based Arduinos (no ADC gain errors measured)
#else
#define SAMPLE_CNT 14
#define SAMPLE_DIVIDER 8
#define SAMPLE_MULTIPLIER 1
#define SAMPLE_OFFSET 5
#endif

static int16_t varVppMeasureVpp(int8_t printValue) {
    int8_t i = 0;
    uint16_t r1 = 0;
    int16_t r2; //correction for ADC gain error

    while (i++ < SAMPLE_CNT) {
        r1 += analogRead(VPP);
    }
    r2 = (r1 / (SAMPLE_DIVIDER * SAMPLE_MULTIPLIER));
#ifdef SAMPLE_OFFSET
    r1+= SAMPLE_OFFSET;
#endif    
    r1 /= SAMPLE_DIVIDER;
#ifdef SAMPLE_SHIFT
    r2 += SAMPLE_SHIFT;
    r1 += r2;
#endif    
    r1 += calOffset;
    if (printValue) {
        uint8_t a = r1%100;
        Serial.print(r1/100);
        Serial.print(F("."));
        if (a < 10) {
            Serial.print(F("0"));          
        }
#if 1
        Serial.println(a);
#else
        //debug - display the voltage skew value in r2
        Serial.print(a);
        Serial.println(F(", "));
        Serial.println(r2);
#endif        
    }
    return r1;
}

// Returns 1 on Success, 0 on Failure
static uint8_t varVppCalibrateVpp(void) {
    uint8_t vppIndex = 0;
    uint8_t i = 1;
    int16_t v = 900; //starting at 9.00 V
    int16_t r1 = 0;
    int16_t r2;
    int16_t minDif;

    Serial.print(F("VPP calib. offset: "));
    Serial.println(calOffset);

    varVppSetVppIndex(1);
    delay(300); //settle voltage

    while (1) {
      // reset the 'minimal difference' variable every time we search for a new index
        minDif = 9000;
        //the 'vppWiper' storage for tap indices is 8bit wide, therefore we must not use tap index 256.
        while (i <= 0xFF) {
            int16_t d1,d2;
            varVppSetVppIndex(i);
            delay(100); //let the voltage settle

            r2 = varVppMeasureVpp(0);
            // Sanity check: the previous voltage can't be higher than the voltage just measured
            // because we are ramping up the voltage. Therefore it must be an ADC measurement
            // glitch. In that case use the previous value as the measured one.
            // This can happen at lower tap indices where the voltage differences are very small (like 0.01V).
            if (r1 > r2) {
              r2 = r1;
            }
            d1 = r1 - v;
            d2 = r2 - v;
            d1 = ABS(d1);
            d2 = ABS(d2);
#if VPP_VERBOSE
            Serial.print(i);
            Serial.print(F(") r2="));
            Serial.print(r2, DEC);
            Serial.print(F(" d1="));
            Serial.print(d1, DEC);
            Serial.print(F(" d2="));
            Serial.print(d2, DEC);
            Serial.print(F(" md="));
            Serial.println(minDif, DEC);
#endif

            if (r2 <= 100) { // less than 1V ? Failure
                r1 = FAIL;
                goto ret;
            }

            if (d2 <= minDif) {
                minDif = d2;
                vppWiper[vppIndex] = i;
                //check last value / voltage
                if (i == 0xFF) {
                    if (v >= 1620 && v <= 1670) {
                        Serial.println(F("*Index for VPP 1650 is 255"));
                        r1 = OK;
                        goto ret;
                    }
                    r1 = FAIL;
                    goto ret;
                }
            } else {
                i--;
                minDif = 5000;
                Serial.print(F("*Index for VPP "));
                Serial.print(v);
                Serial.print(F(" is "));
                Serial.println(i);
                break;
            }
            
            r1 = r2;
            i++;
        }
        vppIndex++;
#if VPP_VERBOSE
        Serial.print(F("vppIndex "));
        Serial.println(vppIndex);
#endif
        if (vppIndex >= MAX_WIPER) {
            r1 = OK;
            goto ret; 
        }
        v += 50; //next voltage to search for is 0.5V higher
    }

ret:
    varVppSet(VPP_5V0);
    return r1;

}

static void varVppStoreWiperCalib() {
    uint8_t i = 0;
    //sanity check
    if (vppWiper[0] == 0) {
        return;
    }

    //write Afterburner calibration header
    EEPROM.update(0, 0xAF);
    EEPROM.update(1, 0xCA);
    EEPROM.update(2, (uint8_t) calOffset);
    while (i < MAX_WIPER) {
        EEPROM.update(3 + i, vppWiper[i]);
        i++;
    }
}


//return 1 on success (variable VPP functionality present), 0 on failure (VPP not detected on board)
static int8_t varVppInit(void) {
    analogReference(ANALOG_REF_EXTERNAL); //use 3V3 external reference
    analogRead(VPP);            // Perform a dummy conversion referring to the datasheet

    wiperStat = 0; //wiper disabled
    mcp4131_init();
    if (mcp4131_detect()) {
#if VPP_VERBOSE
        Serial.print(mcp4131_detected ? F("MCP4131") : F("MCP4151"));
        Serial.println(F(" POT found"));
#endif
        return OK;
    } else {
#if VPP_VERBOSE
        Serial.println(F("POT not found"));
#endif
        return FAIL;
    }
}

//return 1 on success (VPP calibration appears correct), 0 on failure
static int8_t varVppCheckCalibration(void) {
    int16_t v;
    
    varVppReadCalib();
    if (vppWiper[0] == 0) {
        Serial.println(F("I: VPP not calibrated"));
        return FAIL;
    }
    
#if 0
    // This shoots the VPP to 9V - in theory no GALs should have an issue with that voltage.
    // Also, the On switch should be turned off, preventing VPP to reach the GAL pins.
    // check actual voltage 
    varVppSet(VPP_9V0);
    delay(200); //Settle voltage
    v = varVppMeasureVpp(0);
    varVppSet(VPP_5V0); //set VPP back to 5V
    // lower voltages have a good resolution, so we can have a tight voltage check bounds
    if (v < 890 || v > 910) {
        Serial.print(F("ER: VPP voltage check of 9V failed. Expected 900, measured "));
        Serial.println(v);
        return FAIL;
    }
#endif
    return OK;
}

// only for VPP testing and debugging
static void varrVppTestRamp(void) {
    uint8_t i = 1;

    while (i < 256) {
      varVppSetVppIndex(i);
      delay(400);
      Serial.println(i, DEC);
      varVppMeasureVpp(1);
      i++;
    }
    delay(2000);
    varVppSetVppIndex(0);


}

static int8_t varVppCalibrate(void) {
#if 0 
    // only for testing and debugging
    varrVppTestRamp();
    return OK;
#endif

    if (varVppCalibrateVpp()) {
        varVppStoreWiperCalib();
    } else {
        Serial.println(F("ER: Wiper calibration failed"));
        return FAIL;
    }
    return OK;
}
#endif
