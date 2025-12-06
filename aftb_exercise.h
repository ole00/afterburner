#pragma once
#include "Arduino.h"

static const uint8_t exeArduPins24[] PROGMEM = {
    0, 2,  // Pin 1 = D2
    // pins 1 to 8 are set via shift register
    9, 7,  // Pin 10 = D7
    10, 8,  // Pin 11 = D8
            // Pin 12 = GND
    12, 6,  // Pin 13 = D6
    13, 5,  // Pin 14 = D5
    14, 9,  // Pin 15 = D9
    15, 10, // Pin 16 = D10
    16, 11, // Pin 17 = D11
    17, 12, // Pin 18 = D12
    18,  4, // Pin 19 = D4
    // pins 20, 21 and 22 are not controlled by Excersizer, they are set by the GAL chip
    22,  3, // Pin 23 = D3

    0xFF, 0xFF
};

static const uint8_t exeArduPins20[] PROGMEM = {
    0, 2,  // Pin 1 = D2
    // pins 1 to 8 are set via shift register
    10, 9,  // Pin 11 = D9
    11, 10,  // Pin 13 = D10
    12, 11,  // Pin 14 = D11
    13, 12,  // Pin 15 = D12
    14, 4, // Pin 16 = D4
    // pins 15, 15 and 17 are not controlled by Excersizer, they are set by the GAL chip
    18, 3, // Pin 17 = D3

    0xFF, 0xFF
};



void exerciseSetPins(char* line) {
    uint8_t i;
    uint8_t pinCount = 24;
    uint8_t shrZ = 1;
    uint8_t runCnt = 0;
    const uint8_t* arduPins = exeArduPins24;

    // TODO: optimise for speed - do it only once
    if (line[20] == '\r') {
        pinCount = 20;
        arduPins = exeArduPins20;
        pinMode(7, INPUT);
        digitalWrite(PIN_ZIF_GND_CTRL, HIGH); // enable GND pin
        pinMode(8, OUTPUT);
        digitalWrite(8, LOW);
        pinMode(6, OUTPUT);
        digitalWrite(6, LOW);
        pinMode(5, OUTPUT);
        digitalWrite(5, LOW);
    } else {
        Serial.println(line[20], DEC);
        digitalWrite(PIN_ZIF_GND_CTRL, LOW); // disable GND pin
        pinMode(7, OUTPUT);
    }

    // run up to 3 times to handle pulse pins
    while (runCnt < 3) {
        uint8_t pulseCnt = 0;
        i = 1;
        // handle shift register values
        while (i < 9) {
            char d = line[i];
            if (d == '1') {
                lastShiftRegVal |= 1 << (i - 1);
                shrZ = 0;
            } else if (d == '0') {
                lastShiftRegVal &= ~(1 << (i - 1));
                shrZ = 0;
            } else if (d == 'p') { //falling pulse
                // the value depends on the runCnt : 0,2 : High, 1 Low
                if (1 == runCnt)  {
                    lastShiftRegVal &= ~(1 << (i - 1));
                } else {
                    lastShiftRegVal |= 1 << (i - 1);
                }
                pulseCnt++;
                shrZ = 0;
            } else if (d == 'P') { // rising pulse
                // the value depends on the runCnt : 0,2 : Low, 1 High
                if (1 == runCnt)  {
                    lastShiftRegVal |= 1 << (i - 1);
                } else {
                    lastShiftRegVal &= ~(1 << (i - 1));
                }
                pulseCnt++;
                shrZ = 0;
            }
            i++;
        }

        //all pins are in Z state - disable shift register pins
        if (shrZ) {
            digitalWrite(PIN_SHR_EN, HIGH);
        } else {
            digitalWrite(PIN_SHR_EN, LOW);
            setShiftReg(lastShiftRegVal);
        }

        // handle direct pins
        i = 0;
        while (1) {
            uint8_t dataIndex;
            uint8_t arduPin;
            char d;

            dataIndex = pgm_read_byte(&arduPins[i++]);
            if (dataIndex >= pinCount) {
                break;
            }
            arduPin = pgm_read_byte(&arduPins[i++]);

            d = line[dataIndex];
            // set the regular pins (non pulsed) only in the first run
            if (0 == runCnt) {
                if (d == '0' || d == '1') {
                    pinMode(arduPin, OUTPUT);
                    digitalWrite(arduPin, d == '1' ? HIGH : LOW);
                } else if (d == 'z') {
                    pinMode(arduPin, INPUT_PULLUP);
                }
            }
            if (d == 'p' || d == 'P') { // 'p' falling pulse, 'P' rising pulse
                uint8_t level = (d - 'P') >> 5; // div by 32, 'p' starts with High level,  'P' starts with Low level
                // invert the pulse level in the second run to perform the pulse
                if (1 == runCnt) {
                    level = 1 - level;
                }
                pinMode(arduPin, OUTPUT);
                digitalWrite(arduPin, level ? HIGH : LOW);
                pulseCnt++;
            }
            //ignore all other data characters
        }

        // no pin is pulsed -> exit after first run
        if (0 == pulseCnt) {
            break;
        } else {
            // some pins are pulsed, delay between runs
            if (runCnt < 2 && progtime > 0) {
                delay(progtime);
            }
        }
        runCnt++;
    }

}
