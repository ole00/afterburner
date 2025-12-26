#pragma once

// The configuration bellow allows to reduce Afterburner's MCU code size
// if needed, to fit to Arduino boards with reduced flash size.

// Currently, Arduinos based on Atmega32u4 do not fit all supported PLD chips becuase
// its bootloader and serial port code occupies extra 5 kbytes of flash space compared to UNO R3.


// Supports GAL6001 and GAL6002
#define CFG_USE_GAL600X 1

// Supports ATF1502 and AFT1504 - JTAG based
#define CFG_USE_AFT150X 1

// Supports PEEL16CV8
#define CFG_USE_PEEL 1

// Supports Exerciser adapter
#define CFG_USE_EXERCISER 1

// BOARD presets:  it's possible to chose a different combination of supported PLD chips

// =========== Atmega32u4  ==================
#if defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_YUN) || defined(ARDUINO_AVR_MICRO)
#undef CFG_USE_EXERCISER
#endif

