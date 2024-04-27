                                                                      /*                                                               
                                       (banner font: aciiart.eu)
  _____________________________________________________________
 |       _    __ _            _                                \
 |      / \  / _| |_ ___  _ _| |__  _   _ _ __ ___  ___  _ _   |\
 |     / _ \| |_| '_/ _ \| '_/ '_ \| | | | '_/  _ \/ _ \| '_/  ||
 |    / ___ \  _| |_| __/| | | |_) | |_| | | | | | | __/| |    ||
 |   /_/   \_\|  \__\___||_| |____/\___,_|_| |_| |_|___||_|    ||     
 \_____________________________________________________________||
   '------------------------------------------------------------'

   Afterburner: GAL IC Programmer for Arduino by -= olin =-

   Based on ATFblast 3.1 by Bruce Abbott 
     http://www.bhabbott.net.nz/atfblast.html
   
   Based on GALBLAST by Manfred Winterhoff
     http://www.armory.com/%7Erstevew/Public/Pgmrs/GAL/_ClikMe1st.htm

   Based on GALmate by Yorck Thiele
     https://www.ythiee.com/2021/06/06/galmate-hardware/

   Supports:
   * National GAL16V8
   * Lattice GAL16V8A, GAL16V8B, GAL16V8D
   * Lattice GAL22V10B
   * Lattice GAL20V8
   * Atmel ATF16V8B, ATF16V8C, ATF22V10B, ATF22V10CQZ 

   Requires:
   * afterburner PC program to upload JED fuse map, erase, read etc.
   * simple programming circuit. See: https://github.com/ole00/afterburner

   * 2024-02-02 Fixed: Command 'B9' (Calibration Offset = 0,25V) doesn't work
                Note: Also requires elimination of a  in the PC program afterburner.c
                Added: 10.0V measurement in measureVppValues(()
                                                                       */


#define VERSION "0.6.0"

//#define DEBUG_PES
//#define DEBUG_VERIFY

//ARDUINO UNO pin mapping
//    GAL PIN NAME | ARDUINO UNO PIN NUMBER

//programing voltage control pin
#define PIN_VPP        11
#define PIN_SDOUT      12
#define PIN_STROBE     13
#define PIN_PV         9
#define PIN_SDIN       8


#define PIN_RA0         10
#define PIN_RA1         2
#define PIN_RA2         3
#define PIN_RA3         4
#define PIN_RA4         5
#define PIN_RA5         6
#define PIN_SCLK        7

// pin multiplex: ZIF_PIN <----> ARDUINO PIN or Shift register pin (0b1xxx)
#define PIN_ZIF3          2
#define PIN_ZIF4          0b1
#define PIN_ZIF5          0b1000
#define PIN_ZIF6          0b100
#define PIN_ZIF7          0b10
#define PIN_ZIF8          5
#define PIN_ZIF9          6
#define PIN_ZIF10         7
#define PIN_ZIF11         8
#define PIN_ZIF13         12
#define PIN_ZIF14         11
#define PIN_ZIF15         10
#define PIN_ZIF16         9
#define PIN_ZIF20         0b100000
#define PIN_ZIF21         0b10000
#define PIN_ZIF22         4
#define PIN_ZIF23         3
#define PIN_ZIF_GND_CTRL  13

#if CONFIG_IDF_TARGET_ESP32S2 == 1
//A0: VPP sense
//A3: DIGI_POT CS
#define A0   14
#define A1   15
#define A2   16
#define A3   17
//clk and dat is shared SPI bus
#define A4  18
#define A5  21
#endif

// AVR, or UNO R4
//A0: VPP sense
//A3: DIGI_POT CS
#define PIN_SHR_EN   A1
#define PIN_SHR_CS   A2
//clk and dat is shared SPI bus
#define PIN_SHR_CLK  A4
#define PIN_SHR_DAT  A5

#define COMMAND_NONE 0
#define COMMAND_UNKNOWN 1
#define COMMAND_IDENTIFY_PROGRAMMER '*'
#define COMMAND_HELP 'h'
#define COMMAND_UPLOAD 'u'
#define COMMAND_DEBUG 'd'
#define COMMAND_READ_PES 'p'
#define COMMAND_WRITE_PES 'P'
#define COMMAND_READ_FUSES 'r'
#define COMMAND_WRITE_FUSES 'w'
#define COMMAND_VERIFY_FUSES 'v'
#define COMMAND_ERASE_GAL 'c'
#define COMMAND_ERASE_GAL_ALL '~'
#define COMMAND_UTX '#'
#define COMMAND_ECHO 'e'
#define COMMAND_TEST_VOLTAGE 't'
#define COMMAND_SET_GAL_TYPE 'g'
#define COMMAND_ENABLE_CHECK_TYPE 'f'
#define COMMAND_DISABLE_CHECK_TYPE 'F'
#define COMMAND_ENABLE_SECURITY 's'
#define COMMAND_ENABLE_APD 'z'
#define COMMAND_DISABLE_APD 'Z'
#define COMMAND_MEASURE_VPP 'm'
#define COMMAND_CALIBRATE_VPP 'b'
#define COMMAND_CALIBRATION_OFFSET 'B'
#define COMMAND_JTAG_PLAYER 'j'

#define READGAL 0
#define VERIFYGAL 1
#define READPES 2
#define SCLKTEST 3
#define WRITEGAL 4
#define ERASEGAL 5
#define ERASEALL 6
#define BURNSECURITY 7
#define WRITEPES 8
#define VPPTEST 9
#define INIT 100

//check GAL type before starting an operation
#define FLAG_BIT_TYPE_CHECK (1 << 0)

// ATF16V8C flavour
#define FLAG_BIT_ATF16V8C (1 << 1)

// Keep the power-down feature enabled for ATF C GALs
#define FLAG_BIT_APD (1 << 2)

// contents of pes[3]
// Atmel PES is text string eg. 1B8V61F1 or 3Z01V22F1
//                                 ^           ^
#define LATTICE 0xA1
#define NATIONAL 0x8F
#define SGSTHOMSON 0x20
#define ATMEL16 'V'
#define ATMEL22 '1'
#define ATMEL750 'C'

typedef enum {
  UNKNOWN,
  GAL16V8,
  GAL18V10,
  GAL20V8,
  GAL20RA10,
  GAL20XV10,
  GAL22V10,
  GAL26CV12,
  GAL26V12,
  GAL6001,
  GAL6002,
  ATF16V8B,
  ATF20V8B,
  ATF22V10B,
  ATF22V10C,
  ATF750C,
  LAST_GAL_TYPE //dummy
} GALTYPE;

typedef enum {
  PINOUT_UNKNOWN,
  PINOUT_16V8,
  PINOUT_18V10,
  PINOUT_20V8,
  PINOUT_22V10,
  PINOUT_600,
} PINOUT;

#define BIT_NONE 0
#define BIT_ZERO 1
#define BIT_ONE  2

// config bit numbers

#define CFG_BASE_16   2048
#define CFG_BASE_18   3456
#define CFG_BASE_20   2560
#define CFG_BASE_20RA 3200
#define CFG_BASE_20XV 1600
#define CFG_BASE_22   5808
#define CFG_BASE_26CV 6344
#define CFG_BASE_26V  7800
#define CFG_BASE_600  8154
#define CFG_BASE_750 14364

#define CFG_STROBE_ROW 0
#define CFG_SET_ROW 1
#define CFG_STROBE_ROW2 3

// Atmel power-down row
#define CFG_ROW_APD 59


// Naive detection of the board's RAM size - for support of big Fuse map:
// PIN_A11 - present on MEGA (8kB) or Leonardo (2.5kB SRAM)
//  _RENESAS_RA_ - Uno R4 (32kB)
#if defined(PIN_A11) || defined(_RENESAS_RA_)
#define RAM_BIG
#endif

//ESP32-S2
#if CONFIG_IDF_TARGET_ESP32S2 == 1
#define RAM_BIG
#endif


// common CFG fuse address map for cfg16V8 and cfg20V8
// the only difference is the starting address: 2048 for cfg16V8 and 2560 for cfg20V8
// total size: 82
static const unsigned char cfgV8[] PROGMEM =
{
      80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
      0,1,2,3,
      145,
      72,73,74,75,76,77,78,79,
      144,
      4,5,6,7,
      112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
};

// common CFG fuse address map for cfg16V8AB and cfg20V8AB
// the only difference is the starting address: 2048 for cfg16V8AB and 2560 for cfg20V8AB
// total size: 82
static const unsigned char cfgV8AB[] PROGMEM =
{
      0,1,2,3,
      145,
      72,73,74,75,
      80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
      76,77,78,79,
      144,
      4,5,6,7,
};

// common CFG fuse address map for cfg18V10
// starting address: 3456
// total size 20
static const unsigned char cfg18V10[] PROGMEM =
{
      1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 17, 16, 19, 18
};

// common CFG fuse address map for cfg20RA10
// starting address: 3200
// total size 10
static const unsigned char cfgRA10[] PROGMEM =
{
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9
};

// common CFG fuse address map for cfg20XV10
// starting address: 1600
// total size 31
static const unsigned char cfgXV10[] PROGMEM =
{
  30,
  28, 29,
  20, 21, 22,
  10, 11, 12, 13, 14,
  0, 1, 2, 3, 4,
  27, 26,
  23, 24, 25,
  19, 18, 17, 16, 15,
  9, 8, 7, 6, 5
};

// common CFG fuse address map for cfg22V10
// starting address: 5808
// total size 20
static const unsigned char cfgV10[] PROGMEM =
{
      1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14,17,16,19,18,
};

// common CFG fuse address map for cfg26CV12
// starting address: 6344
// total size 24
static const unsigned char cfg26CV12[] PROGMEM =
{
      1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 17, 16, 19, 18, 21, 20, 23, 22
};

// common CFG fuse address map for cfg26V12
// starting address: 7800
// total size 48
static const unsigned char cfg26V12[] PROGMEM =
{
      36, 24, 12, 0, 37, 25, 13, 1, 38, 26, 14, 2, 39, 27, 15, 3,
      40, 28, 16, 4, 41, 29, 17, 5, 42, 30, 18, 6, 43, 31, 19, 7,
      44, 32, 20, 8, 45, 33, 21, 9, 46, 34, 22, 10, 47, 35, 23, 11
};

// common CFG fuse address map for cfg6001
// starting address: 8154
// total size 68
static const unsigned char cfg6001[] PROGMEM =
{
  67, 66,
  25, 29, 33, 37, 41, 45, 49, 53, 57, 61,
  60, 56, 52, 48, 44, 40, 36, 32, 28, 24,
  62, 63, 58, 59, 54, 55, 50, 51, 46, 47,
  42, 43, 38, 39, 34, 35, 30, 31, 26, 27,
  2, 5, 8, 11, 14, 17, 20, 23,
  0, 3, 6, 9, 12, 15, 18, 21,
  22, 19, 16, 13, 10, 7, 4, 1,
  64, 65
};

// common CFG fuse address map for cfg6002
// starting address: 8154
// total size 104
static const unsigned char cfg6002[] PROGMEM =
{
  103, 102,
  25, 29, 33, 37, 41, 45, 49, 53, 57, 61,
  60, 56, 52, 48, 44, 40, 36, 32, 28, 24,
  62, 63, 58, 59, 54, 55, 50, 51, 46, 47,
  42, 43, 38, 39, 34, 35, 30, 31, 26, 27,
  101, 100, 99, 98, 97, 96, 95, 94, 93,
  92, 91, 90, 89, 88, 87, 86, 85, 84,
  66, 67, 68, 69, 70, 71, 72, 73, 74,
  75, 76, 77, 78, 79, 80, 81, 82, 83,
  2, 5, 8, 11, 14, 17, 20, 23,
  0, 3, 6, 9, 12, 15, 18, 21,
  22, 19, 16, 13, 10, 7, 4, 1,
  64, 65
};

// TODO: handle those:
/*
30,  // 75: Security?
135, // 70: Powerdown
136, // 71: PinKeeper
137, // 72: reserved1
138, // 73: reserved2
139, // 74: reserved3
*/
static const uint8_t cfgV750[] PROGMEM = {
   0,  3,  6,  9, 12, 15, 18, 21, 24, 27, // S0
   1,  4,  7, 10, 13, 16, 19, 22, 25, 28, // S1
   2,  5,  8, 11, 14, 17, 20, 23, 26, 29, // S2
  31, 35, 39, 43, 47, 51, 55, 59, 63, 67, // S3
  32, 36, 40, 44, 48, 52, 56, 60, 64, 68, // S4
  33, 37, 41, 45, 49, 53, 57, 61, 65, 69, // S5
  34, 38, 42, 46, 50, 54, 58, 62, 66, 70  // S6
};

//   UES     user electronic signature
//   PES     programmer electronic signature (ATF = text string, others = Vendor/Vpp/timing)
//   cfg     configuration bits for OLMCs

// GAL info 
typedef struct
{
    GALTYPE type;
    unsigned char id0,id1;          /* variant 1, variant 2 (eg. 16V8=0x00, 16V8A+=0x1A)*/
    short fuses;                    /* total number of fuses              */
    char pins;                      /* number of pins on chip             */
    char rows;                      /* number of fuse rows                */
    unsigned char bits;             /* number of fuses per row            */
    char uesrow;                    /* UES row number                     */
    short uesfuse;                  /* first UES fuse number              */
    char uesbytes;                  /* number of UES bytes                */
    char eraserow;                  /* row adddeess for erase             */
    char eraseallrow;               /* row address for erase all (also PES) */
    char pesrow;                    /* row address for PES read/write     */
    char pesbytes;                  /* number of PES bytes                */
    char cfgrow;                    /* row address of config bits (ACW)   */
    unsigned short cfgbase;         /* base address of the config bit numbers */
    const unsigned char *cfg;       /* pointer to config bit numbers      */
    unsigned char cfgbits;          /* number of config bits              */
    unsigned char cfgmethod;        /* strobe or set row for reading config */
    PINOUT pinout;
} galinfo_t;

const static galinfo_t galInfoList[] PROGMEM =
{
//                           +fuses          +bits        +uesbytes   +pesrow          +cfgbase
//                           |     +pins     |  +uesrow   |  +eraserow|   +pesbytes    |           +cfg
//   +-- type   + id0 + id1  |     |   +rows |  |   +uesfuse |   +eraseallrow +cfgrow  |           |       + cfgbits          +cfgmethod      +pinout
//   |          |     |      |     |   |     |  |   |     |  |   |    |   |   |        |           |       |                  |               |
    {UNKNOWN,   0x00, 0x00,     0,  0,  0,   0,  0,    0, 0,  0,  0,  0,  8,  0,       0,        NULL    , 0                , 0             , PINOUT_UNKNOWN},
    {GAL16V8,   0x00, 0x1A,  2194, 20, 32,  64, 32, 2056, 8, 63, 62, 58,  8, 60, CFG_BASE_16  , cfgV8AB  , sizeof(cfgV8AB)  , CFG_STROBE_ROW, PINOUT_16V8   },
    {GAL18V10,  0x50, 0x51,  3540, 20, 36,  96, 44, 3476, 8, 61, 60, 58, 10, 16, CFG_BASE_18  , cfg18V10 , sizeof(cfg18V10) , CFG_SET_ROW   , PINOUT_18V10  },
    {GAL20V8,   0x20, 0x3A,  2706, 24, 40,  64, 40, 2568, 8, 63, 62, 58,  8, 60, CFG_BASE_20  , cfgV8AB  , sizeof(cfgV8AB)  , CFG_STROBE_ROW, PINOUT_20V8   },
    {GAL20RA10, 0x60, 0x61,  3274, 24, 40,  80, 40, 3210, 8, 61, 60, 58, 10, 16, CFG_BASE_20RA, cfgRA10  , sizeof(cfgRA10)  , CFG_SET_ROW   , PINOUT_22V10  },
    {GAL20XV10, 0x65, 0x66,  1671, 24, 40,  40, 44, 1631, 5, 61, 60, 58,  5, 16, CFG_BASE_20XV, cfgXV10  , sizeof(cfgXV10)  , CFG_SET_ROW   , PINOUT_22V10  },
    {GAL22V10,  0x48, 0x49,  5892, 24, 44, 132, 44, 5828, 8, 61, 62, 58, 10, 16, CFG_BASE_22  , cfgV10   , sizeof(cfgV10)   , CFG_SET_ROW   , PINOUT_22V10  },
    {GAL26CV12, 0x58, 0x59,  6432, 28, 52, 122, 52, 6368, 8, 61, 60, 58, 12, 16, CFG_BASE_26CV, cfg26CV12, sizeof(cfg26CV12), CFG_SET_ROW   , PINOUT_22V10  },
    {GAL26V12,  0x5D, 0x5D,  7912, 28, 52, 150, 52, 7848, 8, 61, 60, 58, 12, 16, CFG_BASE_26V , cfg26V12 , sizeof(cfg26V12) , CFG_SET_ROW   , PINOUT_22V10  },
    {GAL6001,   0x40, 0x41,  8294, 24, 78,  75, 97, 8222, 9, 63, 62, 96,  8,  8, CFG_BASE_600 , cfg6001  , sizeof(cfg6001)  , CFG_SET_ROW   , PINOUT_600    },
    {GAL6002,   0x44, 0x44,  8330, 24, 78,  75, 97, 8258, 9, 63, 62, 96,  8,  8, CFG_BASE_600 , cfg6002  , sizeof(cfg6002)  , CFG_SET_ROW   , PINOUT_600    },
    {ATF16V8B,  0x00, 0x00,  2194, 20, 32,  64, 32, 2056, 8, 63, 62, 58,  8, 60, CFG_BASE_16  , cfgV8AB  , sizeof(cfgV8AB)  , CFG_STROBE_ROW, PINOUT_16V8   },
    {ATF20V8B,  0x00, 0x00,  2706, 24, 40,  64, 40, 2568, 8, 63, 62, 58,  8, 60, CFG_BASE_20  , cfgV8AB  , sizeof(cfgV8AB)  , CFG_STROBE_ROW, PINOUT_20V8   },
    {ATF22V10B, 0x00, 0x00,  5892, 24, 44, 132, 44, 5828, 8, 61, 62, 58, 10, 16, CFG_BASE_22  , cfgV10   , sizeof(cfgV10)   , CFG_SET_ROW   , PINOUT_22V10  },
    {ATF22V10C, 0x00, 0x00,  5892, 24, 44, 132, 44, 5828, 8, 61, 62, 58, 10, 16, CFG_BASE_22  , cfgV10   , sizeof(cfgV10)   , CFG_SET_ROW   , PINOUT_22V10  },
    {ATF750C,   0x00, 0x00, 14499, 24, 84, 171, 84,14435, 8, 61, 60,127, 10, 16, CFG_BASE_750 , cfgV750  , sizeof(cfgV750)  , CFG_STROBE_ROW2, PINOUT_22V10 }, // TODO: not all numbers are clear
};
galinfo_t galinfo __attribute__ ((section (".noinit"))); //preserve data between resets

#ifdef RAM_BIG
// for ATF750C
// MAXFUSES = (((171 * 84 bits)  + uesbits + (10*3 + 1 + 10*4 + 5)) + 7) / 8
//               (14504 + 7) / 8 = 1813
#define MAXFUSES 1813
#else
// Boards with small RAM (< 2.5kB) do not support ATF750C
// MAXFUSES calculated as the biggest required space to hold the fuse bitmap
// MAXFUSES = GAL6002 8330 bits = 8330/8 = 1041.25 bytes rounded up to 1042 bytes
//#define MAXFUSES 1042
//extra space added for sparse fusemap (just enough to fit erased ATF750C)
#define MAXFUSES 1332
#define USE_SPARSE_FUSEMAP
#endif


GALTYPE gal __attribute__ ((section (".noinit"))); //the gal device index pointing to galInfoList, value is preserved between resets

static short erasetime = 100, progtime = 100;
static uint8_t vpp = 0;

char echoEnabled;
unsigned char pes[12];
char line[32];
short lineIndex;
char endOfLine;
char mapUploaded;
char isUploading;
char uploadError;
unsigned char fusemap[MAXFUSES];
unsigned char flagBits;
char varVppExists;
uint8_t lastShiftRegVal = 0;

static void setFuseBit(unsigned short bitPos);
static unsigned short checkSum(unsigned short n);
static char checkGalTypeViaPes(void);
static void turnOff(void);
static void printFormatedNumberHex2(unsigned char num) ;

#include "aftb_vpp.h"
#include "aftb_sparse.h"
#include "aftb_seram.h"

// share fusemap buffer with jtag
#define XSVF_HEAP fusemap
#include "jtag_xsvf_player.h"

// print some help on the serial console
void printHelp(char full) {
  Serial.println(F("AFTerburner v." VERSION));
  // indication for PC software that the new board desgin is used
  if (varVppExists) {
    Serial.println(F(" varVpp "));
  }
#ifdef RAM_BIG
    Serial.println(F(" RAM-BIG "));
#endif

  if (!full) {
    Serial.println(F("type 'h' for help"));
    return;
  }
  Serial.println(F("commands:"));
  Serial.println(F("  h - print help"));
  Serial.println(F("  e - toggle echo")); 
  Serial.println(F("  p - read & print PES"));
  Serial.println(F("  r - read & print fuses"));  
  Serial.println(F("  u - upload fuses"));
  Serial.println(F("  w - write uploaded fuses"));
  Serial.println(F("  v - verify fuses"));
  Serial.println(F("  c - erase chip"));
  Serial.println(F("  t - test & set VPP"));
  Serial.println(F("  b - calibrate VPP"));
  Serial.println(F("  m - measure VPP"));
}

static void setFlagBit(uint8_t flag, uint8_t value) {
    if (value) {
        flagBits |= flag;
    } else {
        flagBits &= ~flag;
    }
}

static void setPinMuxUnused(uint8_t pin, uint8_t pm) {
  // set to OUTPUT during active GAL operation and to INPUT when GAL is inactive
  pinMode(pin, pm);
  digitalWrite(pin, LOW);
}

static void setPinMux(uint8_t pm) {
  // ensure pull-up is enabled during reading and disabled when inactive on DOUT pin
  uint8_t doutMode = pm == OUTPUT ? INPUT_PULLUP: INPUT;

  switch (gal) {
  case GAL16V8:
  case ATF16V8B:
    pinMode(PIN_ZIF10, INPUT); //GND via MOSFET
    pinMode(PIN_ZIF11, INPUT);
    pinMode(PIN_ZIF13, INPUT);
    pinMode(PIN_ZIF14, INPUT);
    pinMode(PIN_ZIF16, doutMode); //DOUT
    // ensure ZIF10 is Grounded via transistor
    digitalWrite(PIN_ZIF_GND_CTRL, pm == OUTPUT ? HIGH: LOW);
    break;        

  case GAL18V10:
    pinMode(PIN_ZIF10, INPUT); //GND via MOSFET
    pinMode(PIN_ZIF11, INPUT);
    pinMode(PIN_ZIF13, INPUT);
    pinMode(PIN_ZIF14, INPUT);
    pinMode(PIN_ZIF9, doutMode); //DOUT
    // ensure ZIF10 is Grounded via transistor
    digitalWrite(PIN_ZIF_GND_CTRL, pm == OUTPUT ? HIGH: LOW);

    //pull down unused pins when active
    setPinMuxUnused(PIN_ZIF15, pm);
    setPinMuxUnused(PIN_ZIF16, pm);
    break;
  
  case GAL20V8:
  case ATF20V8B:
    pinMode(PIN_ZIF10, pm);
    pinMode(PIN_ZIF11, pm);
    pinMode(PIN_ZIF13, pm);
    pinMode(PIN_ZIF14, pm);
    pinMode(PIN_ZIF15, doutMode); //DOUT
    pinMode(PIN_ZIF16, pm);       
    // ensure ZIF10 GND pull is disabled
    digitalWrite(PIN_ZIF_GND_CTRL, LOW);

    //pull down unused pins when active
    setPinMuxUnused(PIN_ZIF14, pm);
    setPinMuxUnused(PIN_ZIF16, pm);
    setPinMuxUnused(PIN_ZIF23, pm);

    break;

  case GAL20RA10:
  case GAL20XV10:
  case GAL22V10:
  case GAL26CV12:
  case GAL26V12:
  case ATF22V10B:
  case ATF22V10C:
  case ATF750C:
    pinMode(PIN_ZIF10, pm);
    pinMode(PIN_ZIF11, pm);
    pinMode(PIN_ZIF13, pm);
    pinMode(PIN_ZIF14, doutMode); //DOUT
    pinMode(PIN_ZIF15, pm);
    pinMode(PIN_ZIF16, pm);       
    // ensure ZIF10 GND pull is disabled
    digitalWrite(PIN_ZIF_GND_CTRL, LOW);

    //pull down unused pins when active
    setPinMuxUnused(PIN_ZIF15, pm);
    setPinMuxUnused(PIN_ZIF16, pm);
    setPinMuxUnused(PIN_ZIF22, pm);
    setPinMuxUnused(PIN_ZIF23, pm);
    break;
  
  case GAL6001:
  case GAL6002:
    pinMode(PIN_ZIF10, pm);
    pinMode(PIN_ZIF11, pm);
    pinMode(PIN_ZIF13, pm);
    pinMode(PIN_ZIF14, doutMode); //DOUT
    pinMode(PIN_ZIF15, pm);
    pinMode(PIN_ZIF16, pm);       
    // ensure ZIF10 GND pull is disabled
    digitalWrite(PIN_ZIF_GND_CTRL, LOW);

    //pull down unused pins when active
    setPinMuxUnused(PIN_ZIF3, pm);
    setPinMuxUnused(PIN_ZIF15, pm);
    setPinMuxUnused(PIN_ZIF16, pm);
    setPinMuxUnused(PIN_ZIF22, pm);
    break;

  }
}

static void setupGpios(uint8_t pm) {

  // Serial input of the GAL chip, output from Arduino
  pinMode(PIN_SDIN, pm);

  pinMode(PIN_STROBE, pm);
  pinMode(PIN_PV, pm);
  pinMode(PIN_RA0, pm);
  pinMode(PIN_RA1, pm);
  pinMode(PIN_RA2, pm);
  pinMode(PIN_RA3, pm);
  pinMode(PIN_RA4, pm);
  pinMode(PIN_RA5, pm);
  pinMode(PIN_SCLK, pm);

  pinMode(PIN_VPP, pm);
  if (varVppExists) {
    pinMode(PIN_ZIF_GND_CTRL, OUTPUT);   
    //disconnect shift register pins (High Z) when pm == Input
    digitalWrite(PIN_SHR_EN, pm == INPUT ? HIGH : LOW);
    setPinMux(pm);
  }
}

#define SHR_SET_BIT(X) digitalWrite(PIN_SHR_CLK, 0); \
                        digitalWrite(PIN_SHR_DAT, (X) ? HIGH : LOW); \
                        digitalWrite(PIN_SHR_CLK, 1)
                        
static void setShiftReg(uint8_t val) {
  lastShiftRegVal = val;
  //assume CS is high

  //ensure CLK is high (might be set low by other SPI devices)
  digitalWrite(PIN_SHR_CLK, 1);
  
  // set CS low
  digitalWrite(PIN_SHR_CS, 0);
  SHR_SET_BIT(val & 0b10000000);
  SHR_SET_BIT(val & 0b1000000);
  SHR_SET_BIT(val & 0b100000);
  SHR_SET_BIT(val & 0b10000);
  SHR_SET_BIT(val & 0b1000);
  SHR_SET_BIT(val & 0b100);
  SHR_SET_BIT(val & 0b10);
  SHR_SET_BIT(val & 0b1);
  digitalWrite(PIN_SHR_CS, 1);
}

// setup the Arduino board
void setup() {
// initialize serial:
  Serial.begin(57600);
  isUploading = 0;
  endOfLine = 0;
  echoEnabled = 0;
  mapUploaded = 0;
  lineIndex = 0;
  setFlagBit(FLAG_BIT_TYPE_CHECK, 1); //do type check

  //check & initialise variable voltage (old / new board design)
  varVppExists = varVppInit();

  // shift register
  pinMode(PIN_SHR_EN, OUTPUT);

  // Serial output from the GAL chip, input for Arduino
  pinMode(PIN_SDOUT, INPUT);

  // Set all GPIO pins to Input to prevent accidents when
  // inserting the GAL IC into socket.
  setupGpios(INPUT);

  if (varVppExists) {
    // reads the calibration values
    if (varVppCheckCalibration()) {
      Serial.println(F("I: VPP calib. OK"));
    }
    // set shift reg Chip select
    pinMode(PIN_SHR_CS, OUTPUT);
    digitalWrite(PIN_SHR_CS, 1); //unselect the POT's SPI bus

    //setup serial RAM
    if (seRamInit()) {
      Serial.println(F("I: SeRAM OK"));
    }
  }
  printHelp(0);

  Serial.println(">");
}

static void sparseSetup(char clearArray){
  // Note: Sparse fuse map is ignored on MCUs with big SRAM
  if (gal == ATF750C) {
    sparseInit(clearArray);
  } else {
    sparseDisable();
  }
}

//copy galinfo item from the flash array into RAM backed struct
static void copyGalInfo(void) {
  memcpy_P(&galinfo, &galInfoList[gal], sizeof(galinfo_t));

  sparseSetup(0);
}

// read from serial line and discard the data
void readGarbage() {
  while (Serial.available() > 0) {
    Serial.read();
  }  
}

// Reads input from the serial terminal and returns the command
// which is the first character of the entered text. 

char handleTerminalCommands() {
  char c;
  
  while (Serial.available() > 0) {
    c = Serial.read();
    line[lineIndex] = c;
    if (c == '\n' || c == '\r') {
      endOfLine = 1;
    }
    //echo input to output
    else {
      if (!isUploading && echoEnabled) {
        Serial.print(c);
      }
    }
    if (lineIndex >= sizeof(line)- 2) {
      lineIndex = 0;
      readGarbage();
      Serial.println();
      Serial.println(F("Error: line too long."));
    } else {
      lineIndex++;
    }
  }
  if (endOfLine) {
    c = COMMAND_NONE;

    //single letter command entered
    if (lineIndex == 2) {
      c = line[0];  
    } else if (lineIndex  > 2) {
      c = line[0];  
      if (!isUploading || c != '#') {
        // prevent 2 character commands from being flagged as invalid
        if (!(c == COMMAND_SET_GAL_TYPE || c == COMMAND_CALIBRATION_OFFSET || c == COMMAND_JTAG_PLAYER)) {
          c = COMMAND_UNKNOWN; 
        }
      }
    }
    if (!isUploading) {
      Serial.println();
      line[lineIndex] = 0;
      lineIndex = 0;
    }
    endOfLine = 0;
    return c;
  }
  return COMMAND_NONE; 
}

// Parses decimal integer number typed as 4 or 5 digits.
// Returns the number value.
unsigned short parse45dec(char i, char five) {
  unsigned short v = 0;
  if (five) { //containts 5 digits
    v = (line[i++] - '0') * 10000;
  }
  v += (line[i++] - '0') * 1000;
  v += (line[i++] - '0') * 100;
  v += (line[i++] - '0') * 10;
  v += line[i] - '0';
  return v;
}

// Converts textual hex value 0-9, A-F to a number.
unsigned char toHex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

// Parses hexdecimal integer number typed as 2 digit.
// Returns the number value.
unsigned short parse2hex(char i) {
  if (line[i] == '\r' || line[i] ==  0 || line[i] == ' ') {
    return -1; 
  }
  
  unsigned short v = toHex(line[i++]) << 4;
  return v + toHex(line[i]); 
}

// Parses hexdecimal integer number typed as 4 digit.
// Returns the number value.
unsigned short parse4hex(char i) {
  unsigned short v;
  if (line[i] == '\r' || line[i] ==  0 || line[i] == ' ') {
    return -1; 
  }
  
  v = toHex(line[i++]);
  v <<= 4;
  v |= toHex(line[i++]);
  v <<= 4;
  v |= toHex(line[i++]);
  v <<= 4;
  v |= toHex(line[i]);
  return v ;
}

// Parses a line fed by the serial connection.
// This hnadles a primitive upload protocol that
// expects a programatic data feed - not suitable
// for human interaction.
// Data: each command on its own line
// line starts with '#' character followed by a command
// and a space. Then a command specific data follow.
// Commands:
// t <gal index>: gal type index to the GALTYPEE enum
// f <fuse index> <row>: row of fuse-map data starting on fuse bit index
// c <checksum> : checksum of the whole fuse map
// e : end ofthe upload transfer - returns to terminal

void parseUploadLine() {
  switch (line[1]) {
    case 'e': {
      if (uploadError) {
        Serial.print(F("ER upload failed"));
      } else {
        Serial.print(F("OK upload finished"));
      }
      isUploading = 0;
    } break;

    // gal type
    case 't': {
      short v = line[3] - '0';
      if (v > 0 && v < LAST_GAL_TYPE) {
        gal = (GALTYPE) v;
        copyGalInfo();
        Serial.print(F("OK gal set: "));
        Serial.println((short) gal, DEC);
      } else {
        Serial.println(F("ER unknown gal index"));
        uploadError = 1;
      }
    } break;

    //fusemap data
    case 'f': {
      char i = 8;
      char j;
      unsigned short addr;
      short v;
      char fiveDigitAddr = (line[7] != ' ') ? 1 : 0;

      addr = parse45dec(3, fiveDigitAddr);
      i += fiveDigitAddr;

      do {
        v = parse2hex(i);
        if (v >= 0) {
          for (j = 0; j < 8; j++) {
            // if fuse bit is set -> then change the fusemap
            if (v & (1 << j)) {
              setFuseBit(addr);
            }
            addr++;
          }
          i += 2;
        }
      } while (v >= 0);

      //any fuse being set is considered as uploaded fuse map
      mapUploaded = 1;

      Serial.print(F("OK "));
      Serial.println((short) addr, DEC);
    } break;

    //checksum
    case 'c': {
      unsigned short val = parse4hex(3);
      unsigned char apdFuse = (flagBits & FLAG_BIT_APD) ? 1 : 0;
      unsigned short cs = checkSum(galinfo.fuses + apdFuse);
      if (cs == val) {
        Serial.println(F("OK checksum matches"));
        // Conditioning jed files might not have any fuse set, so as long as
        // they supply empty checksum (C0000) the upload is OK.
        mapUploaded = 1;
      } else {
        uploadError = 1;
        Serial.print(F("ER checksum:"));
        Serial.print(cs, HEX);
        Serial.print(F(" expected:"));
        Serial.println(val, HEX);
      }
    } break;
    
    // PES
    case 'p': {
      uint8_t i = 0;
      uint8_t j = 3;
      while (i < 8) {
        pes[i] = parse2hex(j);
        i++;
        j+=3; //AB:00:...  - 3 characters per one PES byte
      }
    } break;

    default:
      uploadError = 1;
      Serial.println(F("ER unknown upload cmd"));
  }

  lineIndex = 0;
}
// *********************************************************


// set/reset individual pins of GAL 
static void setVCC(char on) {
    //no control for turning the voltage on of
    //it is assumed the voltage is always on
}

static void setVPP(char on) {
    // new board desgin
    if (varVppExists) {
        uint8_t v = VPP_11V0;

        // when PES is read the VPP is not determined via PES
        if (on == READPES) {
            if (gal == ATF16V8B || gal == ATF20V8B || gal == ATF22V10B || gal == ATF22V10C || gal == ATF750C) {
                v = VPP_10V5; 
            } else {
                v = VPP_11V5;
            }
#if 0
            Serial.print(F("VPP index="));
            Serial.println(v);
#endif
        } else {
            //safety check
            if (vpp < 36) {
                vpp = 36; //9V
            } else
            if (vpp > 66) {
                vpp = 48; //12V
            }
            v = (vpp >> 1) - 18; // 18: 2 * 9V, resolution 0.5V (not 0.25V) hence 'vpp >> 1'
#if 0
            Serial.print(F("setVPP "));
            Serial.print(vpp);
            Serial.print(F(" index="));
            Serial.println(v);
#endif
        }
        varVppSet(on ? v : VPP_5V0);
        delay(50); //settle the voltage
    }
    // old board design
    else {
        //programming voltage is controlled by VPP_PIN,
        //but the programming voltage must be set manually by user turning a Pot
        digitalWrite(PIN_VPP, on ? 1 : 0);
        
        //Serial.print(F("VPP set to:"));
        //Serial.println( on ? "12V": "5V");
        delay(10);      
    }

}

static void setSTB(char on) {
  if (varVppExists) {
    const PINOUT p = galinfo.pinout;
    uint8_t pin = PIN_ZIF13;
    if (p == PINOUT_16V8) {
      pin = PIN_ZIF15;
    } else
    if (p == PINOUT_18V10) {
      pin = PIN_ZIF8;
    }
    digitalWrite(pin, on ? 1:0);
  } else {
    digitalWrite(PIN_STROBE, on ? 1:0);
  }
}

static void setPV(char on) {
  if (varVppExists) {
    const PINOUT p = galinfo.pinout;
    uint8_t pin = PIN_ZIF23;

    if (p == PINOUT_22V10) {
      pin = PIN_ZIF3;
    } else
    if (p == PINOUT_20V8) {
      pin = PIN_ZIF22;
    }
    digitalWrite(pin, on ? 1:0);
  } else {
    digitalWrite(PIN_PV, on ? 1:0);
  }
}

static void setSDIN(char on) {
  if (varVppExists) {
    const PINOUT p = galinfo.pinout;
    if (p == PINOUT_18V10) {
      if (on) {
        lastShiftRegVal |= PIN_ZIF7;
      } else {
        lastShiftRegVal &= ~PIN_ZIF7;
      }
      setShiftReg(lastShiftRegVal);
    } else {
    const uint8_t pin = (p == PINOUT_16V8) ? PIN_ZIF9 : PIN_ZIF11;
    digitalWrite(pin, on ? 1:0);
    }
  } else {
    digitalWrite(PIN_SDIN, on ? 1:0);
  }
}

static void setSCLK(char on){
  if (varVppExists) {
    const PINOUT p = galinfo.pinout;
    if (p == PINOUT_18V10) {
      if (on) {
        lastShiftRegVal |= PIN_ZIF6;
      } else {
        lastShiftRegVal &= ~PIN_ZIF6;
      }
      setShiftReg(lastShiftRegVal);
    } else {
    uint8_t pin = (p == PINOUT_16V8) ? PIN_ZIF8 : PIN_ZIF10;
    digitalWrite(pin, on ? 1:0);
    }
  } else {
    digitalWrite(PIN_SCLK, on ? 1:0);
  }
}

// output row address (RA0-5)
static void setRow(char row)
{
  if (varVppExists) {
    uint8_t srval = 0;
    const PINOUT p = galinfo.pinout;
    if (p == PINOUT_16V8) {
      digitalWrite(PIN_ZIF22, (row & 0x1)); //RA0
      digitalWrite(PIN_ZIF3 , (row & 0x2)); //RA1
      if (row & 0x4) srval  |= PIN_ZIF4; //RA2
      if (row & 0x8) srval  |= PIN_ZIF5; //RA3
      if (row & 0x10) srval |= PIN_ZIF6; //RA4
      if (row & 0x20) srval |= PIN_ZIF7; //RA5
    } else 
    if (p == PINOUT_18V10) {
      digitalWrite(PIN_ZIF22, (row & 0x1)); //RA0
      if (row & 0x2) srval  |= PIN_ZIF21; //RA1
      if (row & 0x4) srval  |= PIN_ZIF20; //RA2
      digitalWrite(PIN_ZIF3 , (row & 0x8)); //RA3
      if (row & 0x10) srval  |= PIN_ZIF4; //RA4
      if (row & 0x20) srval  |= PIN_ZIF5; //RA5
    } else
    if (p == PINOUT_22V10 || p == PINOUT_600) {
      if (row & 0x1) srval  |= PIN_ZIF4; //RA0
      if (row & 0x2) srval  |= PIN_ZIF5; //RA1
      if (row & 0x4) srval  |= PIN_ZIF6; //RA2
      if (row & 0x8) srval  |= PIN_ZIF7; //RA3
      digitalWrite(PIN_ZIF8, (row & 0x10)); //RA4
      digitalWrite(PIN_ZIF9, (row & 0x20)); //RA5
    } else { //PINOUT_20V8
      if (row & 0x1) srval  |= PIN_ZIF21; //RA0
      digitalWrite(PIN_ZIF3 , (row & 0x2)); //RA1
      if (row & 0x4) srval  |= PIN_ZIF4; //RA2
      if (row & 0x8) srval  |= PIN_ZIF5; //RA3
      digitalWrite(PIN_ZIF8, (row & 0x10)); //RA4
      digitalWrite(PIN_ZIF9, (row & 0x20)); //RA5           
    }
    setShiftReg(srval);
  } else {
    digitalWrite(PIN_RA0, (row & 0x1));
    digitalWrite(PIN_RA1, ((row & 0x2) ? 1:0));
    digitalWrite(PIN_RA2, ((row & 0x4) ? 1:0));
    digitalWrite(PIN_RA3, ((row & 0x8) ? 1:0));
    digitalWrite(PIN_RA4, ((row & 0x10) ? 1:0));
    digitalWrite(PIN_RA5, ((row & 0x20) ? 1:0));
  }
}

// serial data out form the GAL chip -> received by Arduino
static char getSDOUT(void)
{
  if (varVppExists) {
    const PINOUT p = galinfo.pinout;
    uint8_t pin = PIN_ZIF16;
    
    if (p == PINOUT_22V10 || p == PINOUT_600) {
      pin = PIN_ZIF14;
    } else
    if (p == PINOUT_20V8) {
      pin = PIN_ZIF15;
    } else
    if (p == PINOUT_18V10) {
      pin = PIN_ZIF9;
    }
    return digitalRead(pin) != 0;
  } else {
    return digitalRead(PIN_SDOUT) != 0;
  }
}

// GAL finish sequence
static void turnOff(void)
{
    delay(100);
    setPV(0);    // P/V- low
    setRow(0x3F);// RA0-5 high  
    setSDIN(1);  // SDIN high
    setVPP(0);   // Vpp off (+12V)
    setPV(1);    // P/V- high
    delay(2);
    setVCC(0);   // turn off VCC (if controlled)

    setupGpios(INPUT);
    delay(100); //ensure VPP is low
}

// GAL init sequence
static void turnOn(char mode) {
    setupGpios(OUTPUT);

    if (mode == READPES) {
        mode = 2;      
    } else
    if (
      mode == WRITEGAL ||
      mode == ERASEGAL ||
      mode == ERASEALL ||
      mode == BURNSECURITY ||
      mode == WRITEPES ||
      mode == VPPTEST ||
      mode == READGAL 
    ) {
        mode = 1;
    } else {
        mode = 0;
    }

//     setVPP(mode);
    setVPP(0);    // VPP off
    setPV(0);     // P/V- low
    setRow(0x3F); // RA0-5 high - erase sequence ?
    //setRow(0);    // RA0-5 low
    setSDIN(1);   // SDIN high
    setSCLK(1);   // SCLK high
    setSTB(1);    // STB high
    setVCC(1);    // turn on VCC (if controlled)
    delay(100);
    setSCLK(0);   // SCLK low
    setVPP(mode);
    delay(20);
}


// clock and receive a bit in from GAL SDOUT
static char receiveBit(void)
{
    char b = getSDOUT();
    setSCLK(1);
    setSCLK(0);
    return b;
}

// read n number of bits
static void discardBits(short n)
{
    while (n-- > 0) {
      receiveBit();
    }
}

// clock a bit and send it out to GAL SDIN
static void sendBit(char bitValue, char skipClkLow = 0)
{
    setSDIN(bitValue);
    setSCLK(1);
    // For some reason ATF20V8B needs a slower clock
    if (gal == ATF20V8B) {
      delay(1);
    }
    if (!skipClkLow) {
        setSCLK(0);
    }
}

// send n number of bits to GAL
static void sendBits(short n, char bitValue)
{
    char skipClkLow = flagBits & FLAG_BIT_ATF16V8C;
    while (n-- > 0) {
      sendBit(bitValue, skipClkLow && n == 0);
    }
}

// send row address bits to SDIN 
// ATF22V10C MSb first, other 22V10 LSb first
static void sendAddress(unsigned char n, unsigned char row)
{
  switch (gal) {
  case ATF22V10C:
      while (n-- > 1) {
          sendBit(row & 32);   // clock in row number bits 5-1
          row <<= 1;
      }
      setSDIN(row & 32);       // SDIN = row number bit 0
      break;
  case ATF750C:
      while (n-- > 1) {
          sendBit(row & 1);    // clock in row number bits 0-5
          row >>= 1;
      }
      setSDIN(row & 1);       // SDIN = row number bit 6
      break;
  default:
      while (n-- > 0) {
          sendBit(row & 1);    // clock in row number bits 0-5
          row >>= 1;
      }
      setSDIN(0); // SDIN = low
  }
}


// pulse STB pin low for some milliseconds 
static void strobe(unsigned short msec)
{
  setSTB(0);
  delay(msec);
  setSTB(1);
}

// 16V8, 20V8 RA0-5 = row address, strobe.
// 22V10 RA0-5 = 0, send row address (6 bits), strobe.
// setBit: 0 - do not set bit, 1- set bit value 0, 2 - set bit value 1
static void strobeRow(char row, char setBit = BIT_NONE)
{
  unsigned char nBits = 6;
  switch(gal) {
    case GAL16V8:
    case GAL20V8:
    case ATF20V8B:
    case ATF16V8B:
      setRow(row);         // set RA0-5 to row number
      if (setBit) {
        sendBits(1, setBit - 1);
      }
      strobe(2);           // pulse /STB for 2ms
      break;
    case ATF750C:
      nBits = 7;
      //fall through
    case GAL18V10:
    case GAL20RA10:
    case GAL20XV10:
    case GAL22V10:
    case GAL26CV12:
    case GAL26V12:
    case ATF22V10B:
    case ATF22V10C:
      setRow(0);           // set RA0-5 low
      sendAddress(nBits, row);  // send row number (6 or 7 bits)
      setSTB(0);
      setSTB(1);           // pulse /STB
      setSDIN(0);          // SDIN low
      break;
    case GAL6001:
    case GAL6002:
      setRow(0);
      sendBits(95, 0);
      sendBit(1);
      sendAddress(7, row);
      sendBits(16, 0);
      strobe(2);           // pulse /STB for 2ms
      break;
   }
}

static void strobeConfigRow(char row)
{
  switch(gal) {
    case ATF750C:
      setRow(0);           // set RA0-5 low
      setRow(galinfo.cfgrow);
      sendAddress(7, row);  // send row number (6 bits)
      setSDIN(1);          // SDIN high
      setSTB(0);
      setSTB(1);           // pulse /STB
      break;
   }
}

// read PES: programmer electronic signature (ATF = text string, others = Vendor/Vpp/timing)
static void readPes(void) {
  unsigned short bitmask;
  short byteIndex;

#ifdef DEBUG_PES 
  Serial.print(F("testing gal "));
  Serial.print(gal, DEC);
  Serial.println();
#endif
  turnOn(READPES);

  strobeRow(galinfo.pesrow);

  if (gal == ATF16V8B) {
    setPV(1); //Required for ATF16V8C
  }

  if (gal == GAL6001 || gal == GAL6002) {
    discardBits(20);
  }
  
  for(byteIndex = 0; byteIndex < galinfo.pesbytes; byteIndex++) {
    unsigned char value = 0;
    
    for (bitmask = 0x1; bitmask <= 0x80; bitmask <<= 1) {
      if (receiveBit()) {
       value |= bitmask;
      }
    }
    pes[byteIndex] = value;
  }

  turnOff();
}

static void writePes(void) {
  uint8_t rbit;
  uint8_t b, p;

  if (gal == ATF16V8B || gal == ATF20V8B || gal == ATF22V10B || gal == ATF22V10C) {
    Serial.println(F("ER write PES not supported"));
    return;
  }

  turnOn(WRITEPES);

  setPV(1);

  switch(gal) {
    case GAL6001:
    case GAL6002:
      setRow(0);
      sendBits(20, 0);
      for (rbit = 0; rbit < 64; rbit++) {
        b = pes[rbit >> 3];
        p = b & (1 << (rbit & 0b111));
        sendBit(p);
      }
      sendBits(11, 0);
      sendBit(1);
      sendAddress(7, galinfo.pesrow);
      sendBits(16, 0);
      setSDIN(0);
      break;
    default:
      setRow(galinfo.pesrow);
      for (rbit = 0; rbit < 64; rbit++) {
        b = pes[rbit >> 3];
        p = b & (1 << (rbit & 0b111));
        sendBit(p);
      }
      break;
  }

  strobe(progtime);

  turnOff();
}

static const unsigned char PROGMEM duration[] = {
  1, 2, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 200
};

static unsigned char getDuration(unsigned char index) {
  if (index > 13) {
    return 0;
  }
  return pgm_read_byte(&duration[index]);
}

static void setGalDefaults(void) {
    if (gal == ATF16V8B || gal == ATF20V8B || gal == ATF22V10B || gal == ATF22V10C || gal == ATF750C) {
        progtime = 20;
        erasetime = 100;
        vpp = 42; /* 10.5V */
    } else {
        progtime = 80;
        erasetime = 80;
        vpp = 48; /* 12V */
    }
}

void parsePes(char type) {
  unsigned char algo;

  if (UNKNOWN == type) {
    type = gal; 
  }

#ifdef DEBUG_PES
  Serial.print(F("Parse pes. gal="));
  Serial.println(type, DEC);
#endif

  switch (type) {
    case ATF16V8B:
    case ATF20V8B:
    case ATF22V10B:
    case ATF22V10C:
    case ATF750C:
        progtime = 20;
        erasetime = 100;
        vpp = 48;    /* 12.0V */
    break;
    default:
        algo = pes[1] & 0x0F;
        if (algo == 5) {
            erasetime = (25 << ((pes[4] >> 2) &7)) / 2;
            progtime = getDuration(((((unsigned short)pes[5] << 8)| pes[4]) >> 5) & 15);
            vpp = 2 * ((pes[5] >> 1) & 31) + 20;
        }
        else switch(type) {
        case GAL16V8:
        case GAL20V8:
          erasetime=100;
          goto more;
        case GAL6001:
        case GAL6002:
          erasetime=50;
        more:
          switch(algo) {
            case 0:
                vpp = 63; // 15.75V
                progtime = 100;
                break;
            case 1:
                vpp = 63; // 15.75V
                progtime = 80;
                break;
            case 2:
                vpp = 66; // 16.5V
                progtime = 10;
                break;
            case 3:
                vpp = (pes[3] == NATIONAL) ? 60 : 58; // 15.0V or 14.5V
                progtime = 40;
                break;
            case 4:
                vpp = 56; // 14V
                progtime = 100;
                break;
            }
            break;
        default:
            erasetime = (pes[3] == NATIONAL) ? 50 : 100;
            switch(algo) {
            case 0:
                vpp = 66; // 16.5V
                progtime = 10;
                break;
            case 1:
                vpp = 63; // 15.75V
                progtime = 100;
                break;
            case 2:
                vpp = (pes[3] == NATIONAL) ? 60 : 58; // 15.0V or 14.5V
                progtime = 40;
                break;
            case 3:
                vpp = 56; // 14V
                progtime = 100;
                break;
            }
        }
    }

    //Afterburnes seems to work with programming voltages reduced by 1V
    vpp -= 4; // -1V
}

// print PES information
void printPes(char type) {
  
  Serial.print(F("PES info: "));
  //voltage
  if (pes[3] == ATMEL16 || pes[3] == ATMEL22 || pes[3] == ATMEL750) {
     //Serial.print("  ");
  } else {
    if (pes[1] & 0x10) {
      Serial.print(F("3.3V "));
    } else {
      Serial.print(F("5V "));
    }
  }

  //manufacturer
  switch (pes[3]) {
    case LATTICE:    Serial.print(F("Lattice ")); break;
    case NATIONAL:   Serial.print(F("National ")); break;
    case SGSTHOMSON: Serial.print(F("ST Microsystems ")); break;
    case ATMEL750:
    case ATMEL16:
    case ATMEL22:    Serial.print(F("Atmel ")); break;
    default:         Serial.print(F("Unknown GAL, "));
  }

  // GAL type
  switch (type) {
    case GAL16V8: Serial.print(F("GAL16V8 ")); break;
    case GAL18V10: Serial.print(F("GAL18V10 ")); break;
    case GAL20V8: Serial.print(F("GAL20V8 ")); break;
    case GAL20RA10: Serial.print(F("GAL20RA10 ")); break;
    case GAL20XV10: Serial.print(F("GAL20XV10 ")); break;
    case GAL22V10: Serial.print(F("GAL22V10 ")); break;
    case GAL26CV12: Serial.print(F("GAL26CV12 ")); break;
    case GAL26V12: Serial.print(F("GAL26V12 ")); break;
    case GAL6001: Serial.print(F("GAL6001 ")); break;
    case GAL6002: Serial.print(F("GAL6002 ")); break;
    case ATF16V8B: Serial.print(0 == (flagBits & FLAG_BIT_ATF16V8C) ? F("ATF16V8B "): F("ATF16V8C ")); break;
    case ATF20V8B: Serial.print(F("ATF20V8B ")); break;
    case ATF22V10B: Serial.print(F("ATF22V10B ")); break;
    case ATF22V10C: Serial.print(F("ATF22V10C ")); break;
    case ATF750C: Serial.print(F("ATF750C ")); break;
  }

  //programming info
  if (UNKNOWN != type) {
    Serial.print(F(" VPP="));
    Serial.print(vpp >> 2, DEC);
    Serial.print(F("."));
    Serial.print((vpp & 3) * 25, DEC);
    Serial.print(F(" Timing: prog="));
    Serial.print(progtime, DEC);
    Serial.print(F(" erase="));
    Serial.print(erasetime / 4, DEC);
  } else {
    Serial.print(F(" try VPP=10..14 in 1V steps"));
  }
  
  Serial.println();
}

// sets a fuse bit on particular position
// expects that the fusemap was cleared (set to zero) beforehand
static void setFuseBit(unsigned short bitPos) {
    uint16_t pos;
    if (sparseFusemapStat) {
      pos = sparseSetFuseBit(bitPos);
    } else {
      pos = bitPos >> 3; //divide the bit position by 8 to get the byte position
    }
    fusemap[pos] |= (1 << (bitPos & 7));
}

// gets a fuse bit from specific fuse position
static char getFuseBit(unsigned short bitPos) {
  uint16_t pos;
  if (sparseFusemapStat) {
    pos = sparseGetFuseBit(bitPos);
    if (pos >= 0xFF00) {
      return pos & 0x1;
    }
  } else {
    pos = bitPos >> 3;
  }
  return (fusemap[pos] & (1 << (bitPos & 7))) ? 1 : 0;
}

static void setFuseBitVal(unsigned short bitPos, char val) {
  if (val) {
    setFuseBit(bitPos);
  }
}

// generic fuse-map reading, fuse-map bits are stored in fusemap array
static void readGalFuseMap(const unsigned char* cfgArray, char useDelay, char doDiscardBits) {
  unsigned short cfgAddr = galinfo.cfgbase;
  unsigned short row, bit;
  unsigned short addr;

  if (flagBits & FLAG_BIT_ATF16V8C) {
      setPV(0);
  }

  for(row = 0; row < galinfo.rows; row++) {
    strobeRow(row); //set address of the row
    if (flagBits & FLAG_BIT_ATF16V8C) {
        setSDIN(0);
        setPV(1);
    }
    for(bit = 0; bit < galinfo.bits; bit++) {
      // check the received bit is 1 and if so then set the fuse map
      if (receiveBit()) {
        addr = galinfo.rows;
        addr *= bit;
        addr += row;
        setFuseBit(addr);
      }
    }
    if (useDelay) {
      delay(useDelay);
    }
    if (flagBits & FLAG_BIT_ATF16V8C) {
        setPV(0);
    }
  }

  // read UES
  strobeRow(galinfo.uesrow);
  if (flagBits & FLAG_BIT_ATF16V8C) {
      setSDIN(0);
      setPV(1);
  }

  if (doDiscardBits) {
    discardBits(doDiscardBits);
  }
  for(bit = 0; bit < galinfo.uesbytes * 8; bit++) {
    if (receiveBit()) {
      addr = galinfo.uesfuse;
      addr += bit;
      setFuseBit(addr);
    }
  }
  if (useDelay) {
    delay(useDelay);
  }
  if (flagBits & FLAG_BIT_ATF16V8C) {
      setPV(0);
  }

  // read CFG
if (galinfo.cfgmethod == CFG_STROBE_ROW2) { //ATF750C
    const uint8_t cfgstroberow = 96;
    const uint8_t cfgrowlen = 10;
    const uint8_t cfgrowcount = (galinfo.cfgbits + (cfgrowlen - 1)) /cfgrowlen;
    uint8_t i;
    for(i = 0; i < cfgrowcount; i++) {
      strobeConfigRow(cfgstroberow + i);
      for(bit = 0; bit < cfgrowlen; bit++) {
        uint8_t absBit = (cfgrowlen * i) + bit;
        if (absBit >= galinfo.cfgbits)
          break;
        if (receiveBit()) {
          unsigned char cfgOffset = pgm_read_byte(&cfgArray[absBit]);
          setFuseBit(cfgAddr + cfgOffset);
        }
      }
      if (useDelay) {
        delay(useDelay);
      }
    }
  } else {
    if (galinfo.cfgmethod == CFG_STROBE_ROW) {
      strobeRow(galinfo.cfgrow);
      if (flagBits & FLAG_BIT_ATF16V8C) {
        setSDIN(0);
        setPV(1);
      }
    }
    else {
      setRow(galinfo.cfgrow);
      strobe(1);
    }
    for(bit = 0; bit < galinfo.cfgbits; bit++) {
      if (receiveBit()) {
        unsigned char cfgOffset = pgm_read_byte(&cfgArray[bit]); //read array byte flom flash
        setFuseBit(cfgAddr + cfgOffset);
      }
    }
  }

  //check APD fuse bit - only for ATF16V8C or ATF22V10C
  if ((flagBits & FLAG_BIT_ATF16V8C) || gal == ATF22V10C) {
    setPV(0);
    if (gal == ATF22V10C) {
      setRow(0);
      sendAddress(6, CFG_ROW_APD);
      strobe(1);
    } else { //ATF16V8C
      setRow(CFG_ROW_APD);
      strobe(1);
      setPV(1);
    }
    setFlagBit(FLAG_BIT_APD, receiveBit());
  }

#if 0
  if (sparseFusemapStat) {
    sparsePrintStat();
  }
#endif
}

static void readGalFuseMap600(const unsigned char* cfgArray) {
  unsigned short row, bit;
  unsigned short addr;

  for (row = 0; row < 78; row++)
  {
      strobeRow(row);
      discardBits(20);
      for (bit = 0; bit < 11; bit++)
          setFuseBitVal(7296 + 78 * bit + row, receiveBit());
      for (bit = 0; bit < 64; bit++)
          setFuseBitVal(114 * bit + row, receiveBit());
      discardBits(24);
  }
  for (row = 0; row < 64; row++)
  {
      sendBits(31, 0);
      for (bit = 0; bit < 64; bit++)
          sendBit(bit != row);
      sendBits(24, 0);
      setSDIN(0);
      strobe(2);
      for (bit = 0; bit < 20; bit++)
          setFuseBitVal(78 + 114 * row + bit, receiveBit());
      discardBits(83);
      for (bit = 0; bit < 16; bit++)
          setFuseBitVal(98 + 114 * row + bit, receiveBit());
  }
  // UES
  strobeRow(galinfo.uesrow);
  discardBits(20);
  addr = galinfo.uesfuse;
  for (bit = 0; bit < 72; bit++)
      setFuseBitVal(addr + bit, receiveBit());
  // CFG
  setRow(galinfo.cfgrow);
  strobe(2);
  addr = galinfo.cfgbase;
  for (bit = 0; bit < galinfo.cfgbits; bit++) {
      unsigned char cfgOffset = pgm_read_byte(&cfgArray[bit]); //read array byte flom flash
      setFuseBitVal(addr + cfgOffset, receiveBit());
  }
}

// generic fuse-map verification, fuse map bits are compared against read bits
static unsigned short verifyGalFuseMap(const unsigned char* cfgArray, char useDelay, char doDiscardBits) {
  unsigned short cfgAddr = galinfo.cfgbase;
  unsigned short row, bit;
  unsigned short addr;
  char fuseBit;   // fuse bit received from GAL
  char mapBit;    // fuse bit stored in RAM
  unsigned short errors = 0;

#ifdef DEBUG_VERIFY
  Serial.print(F("rot f:"));
  Serial.println(rotatedFuseMap, DEC);
#endif

  if (flagBits & FLAG_BIT_ATF16V8C) {
      setPV(0);
  }

  // read fuse rows
  for(row = 0; row < galinfo.rows; row++) {
    strobeRow(row);
    if (flagBits & FLAG_BIT_ATF16V8C) {
        setSDIN(0);
        setPV(1);
    }
    for(bit = 0; bit < galinfo.bits; bit++) {
      addr = galinfo.rows;
      addr *= bit;
      addr += row;
      mapBit = getFuseBit(addr); //bit from RAM
      fuseBit = receiveBit(); // read from GAL
      if (mapBit != fuseBit) {
#ifdef DEBUG_VERIFY
        Serial.print(F("f a="));
        Serial.print(addr, DEC);
#endif
        errors++;
      }
    }
    if (useDelay) {
      delay(useDelay);
    }
    if (flagBits & FLAG_BIT_ATF16V8C) {
      setPV(0);
    }
  }

   // read UES
  strobeRow(galinfo.uesrow);
  if (flagBits & FLAG_BIT_ATF16V8C) {
      setSDIN(0);
      setPV(1);
  } 
  if (doDiscardBits) {
    discardBits(doDiscardBits);
  }
  for(bit = 0; bit < galinfo.uesbytes * 8; bit++) {
    addr = galinfo.uesfuse;
    addr += bit;
    mapBit = getFuseBit(addr);
    fuseBit = receiveBit();
    if (mapBit != fuseBit) {
#ifdef DEBUG_VERIFY
      Serial.print(F("U a="));
      Serial.println(bit, DEC);
#endif
      errors++;
    }
  }
  if (useDelay) {
    delay(useDelay);
  }
  if (flagBits & FLAG_BIT_ATF16V8C) {
      setPV(0);
  }

  if (galinfo.cfgmethod == CFG_STROBE_ROW2) { //ATF750C
    const uint8_t cfgstroberow = 96;
    const uint8_t cfgrowlen = 10;
    const uint8_t cfgrowcount = (galinfo.cfgbits + (cfgrowlen - 1)) /cfgrowlen;
    uint8_t i;
    for(i = 0; i < cfgrowcount; i++) {
      strobeConfigRow(cfgstroberow + i);
      for(bit = 0; bit < cfgrowlen; bit++) {
        uint8_t absBit = (cfgrowlen * i) + bit;
        if (absBit >= galinfo.cfgbits) {
          break;
        }
        mapBit = getFuseBit(cfgAddr + pgm_read_byte(&cfgArray[absBit])); // cfgAddr + cfgOffset
        fuseBit = receiveBit();
        if (mapBit != fuseBit) {
  #ifdef DEBUG_VERIFY
        Serial.print(F("C a="));
        Serial.println(absBit, DEC);
  #endif
          errors++;
        }
      }
      if (useDelay) {
        delay(useDelay);
      }
    }
  } else {
    if (galinfo.cfgmethod == CFG_STROBE_ROW) {
      strobeRow(galinfo.cfgrow);
      if (flagBits & FLAG_BIT_ATF16V8C) {
        setSDIN(0);
        setPV(1);
      }
    } else {
      setRow(galinfo.cfgrow);
      strobe(1);
    }
    for(bit = 0; bit < galinfo.cfgbits; bit++) {
      unsigned char cfgOffset = pgm_read_byte(&cfgArray[bit]); //read array byte flom flash
      mapBit = getFuseBit(cfgAddr + cfgOffset);
      fuseBit = receiveBit();
      if (mapBit != fuseBit) {
  #ifdef DEBUG_VERIFY
        Serial.print(F("C a="));
        Serial.println(bit, DEC);
  #endif
        errors++;
      }
    }
  }

  //verify PD fuse on Atmel's C GALs
  if ((flagBits & FLAG_BIT_ATF16V8C) || gal == ATF22V10C) {
    setPV(0);
    if (gal == ATF22V10C) {
      setRow(0);
      sendAddress(6, CFG_ROW_APD);
      strobe(1);
    } else { //ATF16V8C
      setRow(CFG_ROW_APD);
      strobe(1);
      setPV(1);
    }

    mapBit = (flagBits & FLAG_BIT_APD)? 1 : 0;
    fuseBit = receiveBit();
    if (mapBit != fuseBit) {
#ifdef DEBUG_VERIFY
      Serial.println(F("C pd"));
#endif
      errors++;
    }
  }

  return errors;
}


static unsigned short verifyGalFuseMap600(const unsigned char* cfgArray) {
  unsigned short row, bit;
  unsigned short addr;
  char fuseBit;   // fuse bit received from GAL
  char mapBit;    // fuse bit stored in RAM
  unsigned short errors = 0;

  for (row = 0; row < 78; row++)
  {
      strobeRow(row);
      discardBits(20);
      for (bit = 0; bit < 11; bit++) {
          mapBit = getFuseBit(7296 + 78 * bit + row);
          fuseBit = receiveBit();
          if (mapBit != fuseBit) {
#ifdef DEBUG_VERIFY
            Serial.print(F("f a="));
            Serial.println(7296 + 78 * bit + row, DEC);
#endif
            errors++;
          }
      }
      for (bit = 0; bit < 64; bit++) {
          mapBit = getFuseBit(114 * bit + row);
          fuseBit = receiveBit();
          if (mapBit != fuseBit) {
#ifdef DEBUG_VERIFY
            Serial.print(F("f a="));
            Serial.println(114 * bit + row, DEC);
#endif
            errors++;
          }
      }
      discardBits(24);
  }
  for (row = 0; row < 64; row++)
  {
      sendBits(31, 0);
      for (bit = 0; bit < 64; bit++)
          sendBit(bit != row);
      sendBits(24, 0);
      setSDIN(0);
      strobe(2);
      for (bit = 0; bit < 20; bit++) {
          mapBit = getFuseBit(78 + 114 * row + bit);
          fuseBit = receiveBit();
          if (mapBit != fuseBit) {
#ifdef DEBUG_VERIFY
            Serial.print(F("f a="));
            Serial.println(78 + 114 * row + bit, DEC);
#endif
            errors++;
          }
      }
      discardBits(83);
      for (bit = 0; bit < 16; bit++) {
          mapBit = getFuseBit(98 + 114 * row + bit);
          fuseBit = receiveBit();
          if (mapBit != fuseBit) {
#ifdef DEBUG_VERIFY
            Serial.print(F("f a="));
            Serial.println(98 + 114 * row + bit, DEC);
#endif
            errors++;
          }
      }
  }
  // UES
  strobeRow(galinfo.uesrow);
  discardBits(20);
  addr = galinfo.uesfuse;
  for (bit = 0; bit < 72; bit++) {
      mapBit = getFuseBit(addr + bit);
      fuseBit = receiveBit();
      if (mapBit != fuseBit) {
#ifdef DEBUG_VERIFY
        Serial.print(F("f a="));
        Serial.println(addr + bit, DEC);
#endif
        errors++;
      }
  }
  // CFG
  setRow(galinfo.cfgrow);
  strobe(2);
  addr = galinfo.cfgbase;
  for (bit = 0; bit < galinfo.cfgbits; bit++) {
      unsigned char cfgOffset = pgm_read_byte(&cfgArray[bit]); //read array byte flom flash
      mapBit = getFuseBit(addr + cfgOffset);
      fuseBit = receiveBit();
      if (mapBit != fuseBit) {
#ifdef DEBUG_VERIFY
        Serial.print(F("f a="));
        Serial.println(addr + cfgOffset, DEC);
#endif
        errors++;
      }
  }
  
  return errors;
}

// main fuse-map reading and verification function
// READING: reads fuse rows, UES, CFG from GAL and stores into fusemap bit array RAM.
// VERIFY:  reads fuse rows, UES, CFG from GAL and compares with fusemap bit array in RAM.
static void readOrVerifyGal(char verify)
{
  unsigned short i;
  unsigned char* cfgArray = (unsigned char*) cfgV8;

  //ensure fusemap is cleared before READ operation, keep it for VERIFY operation.
  if (!verify) {
    for (i = 0; i < MAXFUSES; i++) {
      fusemap[i] = 0;
    }
    sparseSetup(1);
  }

  turnOn(READGAL);

  switch(gal)
  {
    case GAL16V8:
    case GAL20V8:
        if (pes[2] == 0x1A || pes[2] == 0x3A) {
          cfgArray = (unsigned char*) cfgV8AB;
        }
        //read without delay, no discard
        if (verify) {
          i = verifyGalFuseMap(cfgArray, 0, 0);
        } else {
          readGalFuseMap(cfgArray, 0, 0);
        }
        break;
      
    case ATF16V8B:
    case ATF20V8B:
    case GAL18V10:
    case GAL20RA10:
    case GAL20XV10:
    case GAL26V12:
    case GAL26CV12:
        cfgArray = (unsigned char*) galinfo.cfg;
        //read without delay, no discard
        if (verify) {
          i = verifyGalFuseMap(cfgArray, 0, 0);
        } else {
          readGalFuseMap(cfgArray, 0, 0);
        }
        break;

    case GAL6001:
    case GAL6002:
        cfgArray = (gal == GAL6001) ? (unsigned char*) cfg6001 : (unsigned char*) cfg6002;
        //read without delay, no discard
        if (verify) {
          i = verifyGalFuseMap600(cfgArray);
        } else {
          readGalFuseMap600(cfgArray);
        }
        break;
        
    case GAL22V10:
    case ATF22V10B:
    case ATF22V10C:
      //read with delay 1 ms, discard 68 cfg bits on ATFxx
      if (verify) {
        i = verifyGalFuseMap(cfgV10, 1, (gal == GAL22V10) ? 0 : 68);
      } else {
        readGalFuseMap(cfgV10, 1, (gal == GAL22V10) ? 0 : 68);
      } 
      break;
    case ATF750C:
      //read with delay 1 ms, discard 107 bits on ATF750C
      if (verify) {
        i = verifyGalFuseMap(galinfo.cfg, 1, galinfo.bits - 8 * galinfo.uesbytes - 1);
      } else {
        readGalFuseMap(galinfo.cfg, 1, galinfo.bits - 8 * galinfo.uesbytes - 1);
      }
  }
  turnOff();

  if (verify && i > 0) {
    Serial.print(F("ER verify failed. Bit errors: "));
    Serial.println(i, DEC);
  }
}

// fuse-map writing function for V8 GAL chips
static void writeGalFuseMapV8(const unsigned char* cfgArray) {
  unsigned short cfgAddr = galinfo.cfgbase;
  unsigned char row, rbit;
  unsigned short addr;
  unsigned char rbitMax = galinfo.bits;
  const unsigned char skipLastClk = (flagBits & FLAG_BIT_ATF16V8C) ? 1 : 0;

  setPV(1);
  // write fuse rows
  for (row = 0; row < galinfo.rows; row++) {
    setRow(row);
    for(rbit = 0; rbit < rbitMax; rbit++) {
      addr = galinfo.rows;
      addr *= rbit;
      addr += row;
      sendBit(getFuseBit(addr), rbit == rbitMax - 1 ? skipLastClk : 0);
    }
    strobe(progtime);
  }

  // write UES
  setRow(galinfo.uesrow);
  for (rbit = 0; rbit < 64; rbit++) {
    addr = galinfo.uesfuse;
    addr += rbit;
    sendBit(getFuseBit(addr), rbit == 63 ? skipLastClk : 0);
  }
  strobe(progtime);

  // write CFG (all ICs use setRow)
  rbitMax = galinfo.cfgbits;
  setRow(galinfo.cfgrow);
  for(rbit = 0; rbit < rbitMax; rbit++) {
    unsigned char cfgOffset = pgm_read_byte(&cfgArray[rbit]); //read array byte flom flash
    sendBit(getFuseBit(cfgAddr + cfgOffset), rbit == rbitMax - 1 ? skipLastClk : 0);
  }
  strobe(progtime);
  setPV(0);

  // disable power-down if the APD flag is not set (only for ATF16V8C)
  if (skipLastClk && (flagBits & FLAG_BIT_APD) == 0) {
    setPV(1);
    strobeRow(CFG_ROW_APD, BIT_ZERO); // strobe row and send one bit with value 0
    setPV(0);
  }
}

// fuse-map writing function for V10 GAL chips
static void writeGalFuseMapV10(const unsigned char* cfgArray, char fillUesStart, char useSdin) {
  unsigned short cfgAddr = galinfo.cfgbase;
  unsigned char row, bit;
  unsigned short addr;
  unsigned short uesFill = galinfo.bits - galinfo.uesbytes * 8;

  setRow(0); //RA0-5 low
  // write fuse rows
  for (row = 0; row < galinfo.rows; row++) {
    for (bit = 0; bit < galinfo.bits; bit++) {
      addr = galinfo.rows;
      addr *= bit;
      addr += row;
      sendBit(getFuseBit(addr));
    }
    sendAddress(6, row);
    setPV(1);
    strobe(progtime);
    setPV(0);
  }

  // write UES
  if (fillUesStart) {
    sendBits(uesFill, 1);
  }
  for (bit = 0; bit < galinfo.uesbytes * 8; bit++) {
    addr = galinfo.uesfuse;
    addr += bit;
    sendBit(getFuseBit(addr));
  }
  if (!fillUesStart) {
    sendBits(uesFill, 1);
  }
  sendAddress(6, galinfo.uesrow);
  setPV(1);
  strobe(progtime);
  setPV(0);
  
  // write CFG
  setRow(galinfo.cfgrow);
  for(bit = 0; bit < galinfo.cfgbits - useSdin; bit++) {
    unsigned char cfgOffset = pgm_read_byte(&cfgArray[bit]); //read array byte flom flash
    sendBit(getFuseBit(cfgAddr + cfgOffset));
  }
  if (useSdin) {
    unsigned char cfgOffset = pgm_read_byte(&cfgArray[19]); //read array byte flom flash
    setSDIN(getFuseBit(cfgAddr + cfgOffset));
  }
  setPV(1);
  strobe(progtime);
  setPV(0);

  if (useSdin && (flagBits & FLAG_BIT_APD) == 0) {
    // disable power-down feature (JEDEC bit #5892)
    setRow(0);
    sendAddress(6, CFG_ROW_APD);
    setPV(1);
    strobe(progtime);
    setPV(0);
  }
}

// fuse-map writing function for ATF750C chips
static void writeGalFuseMapV750(const unsigned char* cfgArray) {
  unsigned short cfgAddr = galinfo.cfgbase;
  unsigned char row, bit;
  unsigned short addr;
  unsigned short uesFill = galinfo.bits - (galinfo.uesbytes * 8) - 1;
  uint8_t cfgRowLen = 10; //ATF750C
  uint8_t cfgStrobeRow = 96; //ATF750C
	
  // write fuse rows
  setRow(0); //RA0-5 low
  delayMicroseconds(20);
  for(row = 0; row < galinfo.rows; row++) {
    for (bit = 0; bit < galinfo.bits; bit++) {
      addr = galinfo.rows;
      addr *= bit;
      addr += row;
      sendBit(getFuseBit(addr));
    }

    sendAddress(7, row);
    setPV(1);
    delayMicroseconds(20);
    strobe(progtime);
    delayMicroseconds(100);
    setPV(0);
    delayMicroseconds(12);
  }


  setRow(0); //RA0-5 low
  sendBits(uesFill, 0); //send X number of 0 bits between fuse rows and UES data

  //write UES
  for (bit = 0; bit < (8 * galinfo.uesbytes); bit++) {
    addr = galinfo.uesfuse;
    addr += bit;
    sendBit(getFuseBit(addr));
  }

  //set 1 bit after UES to 0
  sendBits(1, 0);

  row = galinfo.uesrow;
  sendAddress(7, row);
  setPV(1);
  strobe(progtime);
  setPV(0);
  delay(progtime);

  // write CFG
  uint8_t cfgrowcount = (galinfo.cfgbits + (cfgRowLen - 1)) / cfgRowLen;
  for(uint8_t i = 0; i < cfgrowcount; i++) {
    setRow(0);
    delayMicroseconds(10);
    setRow(galinfo.cfgrow);

    for(bit = 0; bit < cfgRowLen; bit++) {
      uint8_t absBit = bit + (i * cfgRowLen);
      //addr = galinfo.cfgbase - (galinfo[gal].bits * rangeStartRow) + cfgArray[absBit];
      addr = galinfo.cfgbase  + pgm_read_byte(&cfgArray[absBit]);
      uint8_t v = getFuseBit(addr);
      sendBit(v);
    }

    sendAddress(7, i + cfgStrobeRow);
    delayMicroseconds(10);
    setPV(1);
    delayMicroseconds(18);
    strobe(progtime); // 20ms
    delayMicroseconds(32);
    setPV(0);
    delayMicroseconds(12);
    setRow(0);
    delayMicroseconds(12);
  }
  //TODO - read the power down fuse bit state from the fuse map and set it only if needed
  if (1) {
    // disable power-down feature (JEDEC bit #5892)
    setRow(0);
    sendAddress(7, 125);
    setPV(1);
    strobe(progtime);
    setPV(0);
    delay(progtime);
  }
}

// fuse-map writing function for 600x GAL chips
static void writeGalFuseMap600(const unsigned char* cfgArray) {
    unsigned short cfgAddr = galinfo.cfgbase;
    unsigned char row, bit;
    unsigned short addr;

    setRow(0);
    for (row = 0; row < 78; row++)
    {
        sendBits(20, 0);
        for (bit = 0; bit < 11; bit++)
            sendBit(getFuseBit(7296 + 78 * bit + row));
        for (bit = 0; bit < 64; bit++)
            sendBit(getFuseBit(114 * bit + row));
        sendBit(1);
        sendAddress(7, row);
        sendBits(16, 0);
        setSDIN(0);
        setPV(1);
        strobe(progtime);
        setPV(0);
    }
    for (row = 0; row < 64; row++)
    {
        for (bit = 0; bit < 20; bit++)
            sendBit(getFuseBit(78 + 114 * row + bit));
        sendBits(11, 0);
        for (bit = 0; bit < 64; bit++)
            sendBit(bit != row);
        sendBits(8, 0);
        for (bit = 0; bit < 16; bit++)
            sendBit(getFuseBit(98 + 114 * row + bit));
        setSDIN(0);
        setPV(1);
        strobe(progtime);
        setPV(0);
    }
    // UES
    sendBits(20, 0);
    addr = galinfo.uesfuse;
    for (bit = 0; bit < 72; bit++)
        sendBit(getFuseBit(addr + bit));
    sendBits(3, 0);
    sendBit(1);
    sendAddress(7, galinfo.uesrow);
    sendBits(16, 0);
    setSDIN(0);
    setPV(1);
    strobe(progtime);
    setPV(0);
    // CFG
    setRow(galinfo.cfgrow);
    for (bit = 0; bit < galinfo.cfgbits; bit++)
    {
        unsigned char cfgOffset = pgm_read_byte(&cfgArray[bit]); //read array byte flom flash
        sendBit(getFuseBit(cfgAddr + cfgOffset));
    }
    setSDIN(0);
    setPV(1);
    strobe(progtime);
    setPV(0);
}

// main fuse-map writing function
static void writeGal()
{
  unsigned short i;
  unsigned char* cfgArray = (unsigned char*) cfgV8;


  turnOn(WRITEGAL);

  switch(gal)
  {
    case GAL16V8:
    case GAL20V8:
        if (pes[2] == 0x1A || pes[2] == 0x3A) {
          cfgArray = (unsigned char*) cfgV8AB;
        }
        writeGalFuseMapV8(cfgArray); 
        break;
      
    case ATF16V8B:
    case ATF20V8B:
        writeGalFuseMapV8(cfgV8AB); 
        break;

    case GAL6001:
    case GAL6002:
        cfgArray = (unsigned char*) galinfo.cfg;
        writeGalFuseMap600(cfgArray);
        break;

    case GAL18V10:
    case GAL20RA10:
    case GAL20XV10:
    case GAL26CV12:
    case GAL26V12:
        cfgArray = (unsigned char*) galinfo.cfg;
        writeGalFuseMapV10(cfgArray, 0, 0);
        break;
        
    case GAL22V10:
    case ATF22V10B:
    case ATF22V10C:
        writeGalFuseMapV10(cfgV10, (gal == GAL22V10) ? 0 : 1, (gal == ATF22V10C) ? 1 : 0);
        break;
    case ATF750C:
        writeGalFuseMapV750(cfgV750);
  }
  turnOff();
}

// erases fuse-map in the GAL
static void eraseGAL(char eraseAll)
{
    turnOn(ERASEGAL);
    
    setPV(1);
    setRow(eraseAll ? galinfo.eraseallrow : galinfo.eraserow);
    if (gal == GAL16V8 || gal == ATF16V8B || gal==GAL20V8) {
        sendBit(1);
    }
    strobe(erasetime);
    setPV(0);
    turnOff();
}

// sets security bit - disables fuse reading
static void secureGAL(void)
{
    turnOn(WRITEGAL);

    setPV(1);
    strobeRow(61, BIT_ONE); // strobe row and send one bit with value 1

    setPV(0);
    turnOff();
}

static char checkGalTypeViaPes(void)
{
    char type = UNKNOWN;

#ifdef DEBUG_PES
    char i;
    Serial.println(F("PES raw bytes:"));
    for (i = 0; i < 10; i++) {
      printFormatedNumberHex2(pes[i]);
      Serial.print(F(" "));
    }
    Serial.println();
#endif
    setFlagBit(FLAG_BIT_ATF16V8C, 0);
    
    if (pes[7] == 'F' && pes[6]== '2' && pes[5]== '2' && (pes[4]== 'V' || pes[4]=='L') && pes[3]== '1' && pes[2]=='0') {
       if (pes[1] == 'B') {
           type = ATF22V10B;
       } else {
           type = ATF22V10C;
       }
    }
    else if (pes[6] == 'F' && pes[5] == '2' && pes[4]== '0' && pes[3] == 'V' && pes[2]=='8' && pes[1] == 'B') {
      type = ATF20V8B;
    }
    else if (pes[6] == 'F' && pes[5] == '1' && pes[4]== '6' && pes[3] == 'V' && pes[2]=='8') {
       type = ATF16V8B;
       if (pes[1] == 'C' || pes[1] == 'Z') { // ATF16V8C, ATF16V8CZ
           setFlagBit(FLAG_BIT_ATF16V8C, 1);
       }
    }
    else if (pes[8] == 'F' && pes[7] == 'V' && pes[6] == '7' && pes[5] == '5' && pes[4] == '0' && pes[3] =='C') {
      // complete string at beginning of row 127: "300C057VF100"
      type = ATF750C;
    }
    else if (pes[2] != 0x00 && pes[2] != 0xFF) {
       for (type = (sizeof(galInfoList) / sizeof(galinfo_t)) - 1; type; type--) {
           uint8_t id0 = pgm_read_byte(&galInfoList[type].id0);
           uint8_t id1 = pgm_read_byte(&galInfoList[type].id1);
           if (pes[2] == id0 || pes[2] == id1) break;
       }
    } else if (pes[3] == SGSTHOMSON && pes[2] == 0x00) {
       type = GAL16V8;
    }

    return type;
}

// checks whether gal type corresponds to PES information on the IC
// note: PES must be read beforehand
static char testProperGAL(void)
{
    char type = checkGalTypeViaPes();

    if (type == 0) {
       //Unknown or illegal PES,
       goto error;
    }
    else if (type != gal) {
      //PES indicates a different GAL type than selected. Change to detected GAL type?
      goto error;
    }

    return 1;

error:
    Serial.println(F("ER unknown or wrong GAL type (check Power ON)"));
    return 0;
}

// prints a hexadecimal number - 2 digits with a leading zero
static void printFormatedNumberHex2(unsigned char num) {
  if (num < 16) {
    Serial.print(F("0"));
  }
  Serial.print(num, HEX);  
}

// prints a hexadecimal number - 4 digits with a leading zero
static void printFormatedNumberHex4(unsigned short num) {
  if (num < 0x10) {
    Serial.print(F("000"));
  } else
  if (num < 0x100) {
    Serial.print(F("00"));
  } else
  if (num < 0x1000) {
    Serial.print(F("0"));
  }
  Serial.print(num, HEX);  
}

// prints a decimal number - 4 digits with a leading zero
static void printFormatedNumberDec4(unsigned short num) {
  if (num < 1) {
    Serial.print(F("0000"));
    return;
  }

  if (num < 10) {
    Serial.print(F("000"));
  } else
  if (num < 100) {
    Serial.print(F("00"));
  } else
  if (num < 1000) {
    Serial.print(F("0"));
  }
  Serial.print(num, DEC);  
}

// adds a formated decimal number with a leading zero to a line buffer at position 'i'
static unsigned char addFormatedNumberDec4(unsigned short num, unsigned char i) {
  char cnt = 3;

  while (cnt >= 0) {
    line[i + cnt] = '0' + (num % 10);
    num /= 10;
    cnt--;
  }
  return i + 4;
}

// calculates fuse-map checksum and returns it
static unsigned short checkSum(unsigned short n)
{
    unsigned short c, e, i;
    unsigned long a;

    c = e= 0;
    a = 0;
    for (i = 0; i < n; i++) {
        e++;
        if (e == 9) {
          e = 1;
          a += c;
          c = 0;
        }
        c >>= 1;
        if (getFuseBit(i)) {
          c += 0x80;
        }
    }
    return (unsigned short)((c >> (8 - e)) + a);
}

static void printGalName() {
    switch (gal) {
    case GAL16V8: Serial.println(F("GAL16V8")); break;
    case GAL18V10: Serial.println(F("GAL18V10")); break;
    case GAL20V8: Serial.println(F("GAL20V8")); break;
    case GAL20RA10: Serial.println(F("GAL20RA10")); break;
    case GAL20XV10: Serial.println(F("GAL20XV10")); break;
    case GAL22V10: Serial.println(F("GAL22V10")); break;
    case GAL26CV12: Serial.println(F("GAL26CV12")); break;
    case GAL26V12: Serial.println(F("GAL26V12")); break;
    case GAL6001: Serial.println(F("GAL6001")); break;
    case GAL6002: Serial.println(F("GAL6002")); break;
    case ATF16V8B:
        if (flagBits & FLAG_BIT_ATF16V8C) {
            Serial.println(F("ATF16V8C"));          
        } else {
            Serial.println(F("ATF16V8B"));
        }
        break;
    case ATF20V8B: Serial.println(F("ATF20V8B")); break;
    case ATF22V10B: Serial.println(F("ATF22V10B")); break;
    case ATF22V10C: Serial.println(F("ATF22V10C")); break;
    case ATF750C: Serial.println(F("ATF750C")); break;
    default:  Serial.println(F("GAL")); break;
    }
}

static unsigned printJedecBlock(unsigned short k, unsigned short bits, unsigned short rows) {
  unsigned short i, j;
  unsigned char unused;

  for (i = 0; i < bits; i++)
  {
      unused = 1;
      for (j = 0; j < rows; j++)
      {
        unused &= !getFuseBit(k + j);
      }
      if (unused) {
        k += rows;
        continue;
      }

      Serial.print('L');
      printFormatedNumberDec4(k);
      Serial.print(' ');
      for (j = 0; j < rows; j++, k++)
      {
          if (getFuseBit(k)) {
              unused = 0;
              Serial.print('1');
          } else {
              Serial.print('0');
          }
      }
      Serial.println('*');
  }
  return k;
}

// prints the contents of fuse-map array in the form of JEDEC text file
static void printJedec()
{
    unsigned short i, j, k, n;
    unsigned char unused, start;
    uint8_t apdFuse = (flagBits & FLAG_BIT_APD) ? 1 : 0;

    Serial.print(F("JEDEC file for "));
    printGalName();
    Serial.print(F("*QP")); Serial.print(galinfo.pins, DEC);
    Serial.print(F("*QF")); Serial.print(galinfo.fuses + apdFuse, DEC);
    Serial.println(F("*QV0*F0*G0*X0*"));
    
    k = 0;
    if (gal == GAL6001 || gal == GAL6002) {
      k = printJedecBlock(k, 64, 114);
      k = printJedecBlock(k, 11, 78);
    } else {
      k = printJedecBlock(k, galinfo.bits, galinfo.rows);
    }

    if( k < galinfo.uesfuse) {
        Serial.print('L');
        printFormatedNumberDec4(k);
        Serial.print(' ');
        
        while(k < galinfo.uesfuse) {
           if (getFuseBit(k)) {
              unused = 0;
              Serial.print('1');
           } else {
              Serial.print('0');
           }
           k++;
        }
        Serial.println('*');
    }


    // UES in byte form
    Serial.print(F("N UES"));
    for (j = 0;j < galinfo.uesbytes; j++) {
        n = 0;
        for (i = 0; i < 8; i++) {
            if (getFuseBit(k + 8 * j + i)) {
                if (gal == ATF22V10C || gal == ATF750C) {
                    n |= 1 << (7 - i);  // big-endian
                }
                else {
                    n |= 1 << i;     // little-endian
                }
            }
        }
        Serial.print(' ');
        printFormatedNumberHex2(n);  
    }
    Serial.println('*');

    // UES in bit form
    Serial.print('L');
    printFormatedNumberDec4(k);
    Serial.print(' ');
    
    for(j = 0; j < 8 * galinfo.uesbytes; j++) {
      if (getFuseBit(k++)) {
         Serial.print('1');
      } else {
         Serial.print('0');
      }
    }
    Serial.println('*');

    // CFG bits
    if (k < galinfo.fuses) {
      Serial.print('L');
      printFormatedNumberDec4(k);
      Serial.print(' ');

      while( k < galinfo.fuses) {
        if (getFuseBit(k++)) {
           Serial.print('1');
        } else {
           Serial.print('0');
        }
      }
      //ATF16V8C
      if (apdFuse) {
        Serial.print('1');
        setFuseBit(k); // set for correct check-sum calculation
      }
      Serial.println('*');
    } else if (apdFuse) { //ATF22V10C
      Serial.print('L');
      printFormatedNumberDec4(k);
      Serial.println(F(" 1*"));
      setFuseBit(k); // set for correct check-sum calculation
    }

    Serial.print(F("N PES"));
    for(i = 0; i < galinfo.pesbytes; i++) {
        Serial.print(' ');
        printFormatedNumberHex2(pes[i]);  
    }
    Serial.println('*');
    Serial.print('C');
    printFormatedNumberHex4(checkSum(galinfo.fuses + apdFuse));
    Serial.println();
    Serial.println('*');
}

// helper print function to save RAM space
static void printNoFusesError() {
  Serial.println(F("ER fuse map not uploaded"));
}

static void testVoltage(int seconds) {
  int i;

  // New board design: set VPP to 16.5V and measure values 
  // on analogue pin A1
  if (varVppExists) {
    int16_t v;
    uint8_t okCnt = 0;
    
    varVppSetMax();
    for (i = 0 ; i < seconds; i++) {
      delay(1000);
      v = varVppMeasureVpp(1); //measure and print
      if (v >= 1640 && v <= 1664) {
        okCnt++;
        // stop early if the VPP is set correctly (still allow time for POT fine-tuning)
        if (okCnt > 3) {
          Serial.println(F("VPP OK"));
          i = seconds;
        }
      } else {
        okCnt = 0;
      }
    }
    varVppSet(VPP_5V0);
  }
  // Legacy board design: set the VPP_EN pin "On" and check 
  // with multimeter the desired VPP voltage specific for GAL chip.
  else {
    pinMode(PIN_VPP, OUTPUT);
    setVPP(1);
    for (i = 0 ; i < seconds; i++) {
        delay(1000);
    }
    setVPP(0);
    pinMode(PIN_VPP, INPUT);
  }
}


// returns 1 if type check if OK, 0 if gal type does not match the type read from PES
static char doTypeCheck(void) {
  
  if (0 == flagBits & FLAG_BIT_TYPE_CHECK) {
    setGalDefaults();
    return 1; // no need to do type check
  }
  readPes();
  parsePes(UNKNOWN);
  return testProperGAL();
}

static void measureVpp(uint8_t index) {
  varVppSet(index);
  delay(150);
  varVppMeasureVpp(1); //print measured value
  delay(5000);
}

static void measureVppValues(void) {
  if (!varVppExists) {
    Serial.println(F("ER variable VPP not supported"));
    return;
  }
  Serial.print(F("VPP calib. offset: "));
  Serial.println(calOffset);

  Serial.print(F("VPP: 4.2 - 5.0V : "));
  measureVpp(VPP_5V0);

  Serial.print(F("VPP: 9.0V : "));
  measureVpp(VPP_9V0);

  Serial.print(F("VPP: 10.0V : "));
  measureVpp(VPP_10V0);

  Serial.print(F("VPP: 12.0V : "));
  measureVpp(VPP_12V0);

  Serial.print(F("VPP: 14.0V : "));
  measureVpp(VPP_14V0);

  Serial.print(F("VPP: 16.0V : "));
  measureVpp(VPP_16V0);

  varVppSet(VPP_5V0);
}

static void calibrateVpp(void) {
  if (!varVppExists) {
    Serial.println(F("ER variable VPP not supported"));
    return;
  }
  if (varVppCalibrate()) {
    Serial.println(F("Calibration OK"));
  }
}

static void startJtagPlayer(uint8_t vpp) {
  jtag_port_t jport;
  //assign jtag pins
  jport.tms = 12;
  jport.tdi = 2;
  jport.tdo = 4;
  jport.tck = 3;
  jport.vref = 10;

  //Serial.println(vpp ? F("JTAG VPP 1"): F("JTAG VPP 0"));

  // ensure PC app is ready
  delay(200);
  // set VPP if required
  if (varVppExists) {
    varVppSet(vpp ? VPP_11V0 : VPP_5V0);
  }

  // start XSVF player / processor
  jtag_play_xsvf(&jport);

  // unset VPP
  if (varVppExists) {
    varVppSet(VPP_5V0);
  }
}

// Arduino main loop
void loop() {


    // read a command from serial terminal or COMMAND_NONE if nothing is received from serial
    char command = handleTerminalCommands();

    // any unexpected input when uploading fuse map terminates the upload process
    if (isUploading && command != COMMAND_UTX && command != COMMAND_NONE) {
      Serial.println(F("ER upload aborted"));
      isUploading = 0;
      lineIndex = 0;
    }

    // handle commands received from the serial terminal
    switch (command) {
      
      // print some help
      case COMMAND_HELP: {
        printHelp(1);
      } break;

      case COMMAND_IDENTIFY_PROGRAMMER : {
        printHelp(0);
      } break;

      // verify fuse-map bits and bits read from the GAL chip
      case COMMAND_VERIFY_FUSES: {
        if (mapUploaded) {
          if (doTypeCheck()) {
            readOrVerifyGal(1); //just verify, do not overwrite fusemap
          }
        } else {
          printNoFusesError();
        }
      } break;

      // handle upload command - start the download of fuse-map
      case COMMAND_UPLOAD: {
        short i;
        // clean fuses
        for (i = 0; i < MAXFUSES; i++) {
          fusemap[i] = 0;
        }
        sparseSetup(1);
        isUploading = 1;
        uploadError = 0;
      } break;

      // command of the upload protocol
      case COMMAND_UTX : {
        parseUploadLine();
      } break;

      // read and print the PES
      case COMMAND_READ_PES : {
        char type;
        readPes();
        type = checkGalTypeViaPes();
        parsePes(type);
        printPes(type);
      } break;

      case COMMAND_WRITE_PES : {
        char type;
        type = checkGalTypeViaPes();
        parsePes(type);
        writePes();
      } break;

      // read fuse-map from the GAL and print it in the JEDEC form
      case COMMAND_READ_FUSES : {
        if (doTypeCheck()) {
          readOrVerifyGal(0); //just read, no verification
          printJedec();
        }
      } break;

      // write current fuse-map to the GAL chip
      case COMMAND_WRITE_FUSES : {
        if (mapUploaded) {
          if (doTypeCheck()) {
            writeGal();
            //security is handled by COMMAND_ENABLE_SECURITY command
          }
        } else {
          printNoFusesError();
        }
      } break;

      // erases the fuse-map on the GAL chip
      case COMMAND_ERASE_GAL: {
        if (doTypeCheck()) {
          eraseGAL(0);
        }
      } break;
      // erases PES and the fuse-map on the GAL chip
      case COMMAND_ERASE_GAL_ALL: {
        if (doTypeCheck()) {
          eraseGAL(1);
        }
      } break;

      // sets the security bit
      case COMMAND_ENABLE_SECURITY: {
        if (doTypeCheck()) {
          secureGAL();
        }
      } break;

      // keep atmel power-down feature enabled during write
      case COMMAND_ENABLE_APD: {
        setFlagBit(FLAG_BIT_APD, 1);
        Serial.println(F("OK APD set"));
      } break;

      case COMMAND_DISABLE_APD: {
        setFlagBit(FLAG_BIT_APD, 0);
        Serial.println(F("OK APD cleared"));
      } break;

      // toggles terminal echo
      case COMMAND_ECHO : {
        echoEnabled = 1 - echoEnabled;
      } break;

      case COMMAND_TEST_VOLTAGE : {
        testVoltage(20);
      } break;

      case COMMAND_SET_GAL_TYPE : {
        char type = line[1] - '0';
        if (type >= 1 && type < LAST_GAL_TYPE) {
          gal = (GALTYPE) type;
          copyGalInfo();
          if (0 == flagBits & FLAG_BIT_TYPE_CHECK) { //no type check requested
            setGalDefaults();
          }
        } else {
          Serial.print(F("ER Unknown gal type "));
          Serial.println(type, DEC);
        }
      } break;
      case COMMAND_ENABLE_CHECK_TYPE: {
        setFlagBit(FLAG_BIT_TYPE_CHECK, 1);
      } break;
      case COMMAND_DISABLE_CHECK_TYPE: {
        int i = 0;
        while(i < 12){
            pes[i++] = 0;
        }
        setFlagBit(FLAG_BIT_TYPE_CHECK, 0);
      } break;

      case COMMAND_MEASURE_VPP: {
        measureVppValues();
      } break;

      // calibration offset helps to offset the resistor tolerances in voltage dividers and also
      // small differences in analog ref which is ~3.3 V derived from LDO.
      case COMMAND_CALIBRATION_OFFSET: {
        int8_t offset = line[1] - '0';
        if (offset >=0 && offset <= 64) {
          //0:-0.32V 1:-0.31V  2: -0.30V  ... 32:0V  33:0.01V 34: 0.02V ... 64:0.32V
          calOffset = offset - 32;
          Serial.print(F("Using cal offset: "));
          Serial.println(calOffset);
        } else {
          Serial.println(F("ER: cal offset failed"));
        }
      } break;

      case COMMAND_CALIBRATE_VPP: {
        calibrateVpp();
      } break;

      case COMMAND_JTAG_PLAYER: {
        startJtagPlayer(line[1] == '1');
        //flush the serial line in case the player ended abruptly
        readGarbage();
      } break;

      default: {
        if (command != COMMAND_NONE) {
          Serial.print(F("ER Unknown command: "));
          Serial.println(line);
        }
      }
    }

    // display prompt character - important for the PC program to check that Arduino
    // finished the desired operation
    if (command != COMMAND_NONE) {
      Serial.println(F(">"));
    }

    // and that's it!
}
