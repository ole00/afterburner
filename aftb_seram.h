#ifndef _AFTB_SERAM_
#define _AFTB_SERAM_

 /* Serial RAM functions for Afterburner GAL project.
 Uses 23LC512 or 23LC1024 RAM IC

3 devices are connected to the serial bus: digi-pot, shit register and this serial RAM.
Digi pot and shift register have their own dedicated CS pins. Serial RAM CS is active (low)
when no other device is selected. Therefore, the serial RAM is always selected unless any other
device is explicitely selected (in that case serial RAM is de-selected by onboard HW)

Reading or Writing of 1 byte takes ~ 620 uSec for 24bit addressing and ~ 500 uSec for 16bit addressing

 */

//set default pins
#ifndef SHR_CS
#define SHR_CS   A2
#endif

#ifndef RAM_CLK
#define RAM_CLK  A4
#endif

#ifndef RAM_DAT
#define RAM_DAT  A5
#endif

#define CS_DELAY_US 16

#define OPCODE_WRITE 2
#define OPCODE_READ 3
#define OPCODE_RDMR 5
#define OPCODE_WRMR 1

#ifndef RAM_BIG

#define seRamInit() 0

#else /* RAM_BIG */

uint8_t ramAddrBits24 = 0;

static void seRamWriteData(uint16_t data, uint8_t bitLen ) {
  uint16_t  mask = (1 << (bitLen-1));

  while (bitLen) {
    bitLen--;
    //set data bit
    digitalWrite(RAM_DAT, (data & mask) ? 1 : 0 );
    //raise the clock
    digitalWrite(RAM_CLK, 1);
    //do some operation
    data <<= 1;
    //lower the clock
    digitalWrite(RAM_CLK, 0);
  }
}

static uint8_t seRamReadData(void) {
  uint8_t bitLen = 8;
  uint8_t result = 0;

  while (bitLen) {
    result <<= 1;
    //raise the clock
    digitalWrite(RAM_CLK, 1);
    //set data bit
    result |= digitalRead(RAM_DAT);
    //do some operation
    bitLen--;
    //lower the clock
    digitalWrite(RAM_CLK, 0);
  }
  return result;
}

static void seRamWrite(uint16_t addr, uint8_t data ) {
    //ensure clock is low
  digitalWrite(RAM_CLK, 0);

  // toggle the SHR CS to reset the bus for serial RAM
  digitalWrite(SHR_CS, 0);
  delayMicroseconds(CS_DELAY_US);
  digitalWrite(SHR_CS, 1);

  seRamWriteData(OPCODE_WRITE, 8); // 8 bits of WRITE opcode
  if (ramAddrBits24) {
    seRamWriteData(0, 8); // top 8 bit of address are 0
  }
  seRamWriteData(addr, 16); // 16 bits of address
  seRamWriteData(data, 8); // 8 bits of actual data
}

static uint8_t seRamRead(uint16_t addr) {
  uint8_t data;
  //ensure clock is low
  digitalWrite(RAM_CLK, 0);

  // toggle the SHR CS to reset the bus for serial RAM
  digitalWrite(SHR_CS, 0);
  delayMicroseconds(CS_DELAY_US);
  digitalWrite(SHR_CS, 1);

  seRamWriteData(OPCODE_READ, 8); // 8 bits of READ opcode
  if (ramAddrBits24) {
    seRamWriteData(0, 8); // top 8 bit of address are 0
  }
  seRamWriteData(addr, 16); // 16 bits of address
  pinMode(RAM_DAT, INPUT);
  data = seRamReadData();
  pinMode(RAM_DAT, OUTPUT);
  return data;
}

static void seRamSetupMode(void) {
  uint8_t data;
  //ensure clock is low
  digitalWrite(RAM_CLK, 0);

  // toggle the SHR CS to reset the bus for serial RAM
  digitalWrite(SHR_CS, 0);
  delayMicroseconds(CS_DELAY_US);
  digitalWrite(SHR_CS, 1);

  seRamWriteData(OPCODE_RDMR, 8); // 8 bits of Read Mode register
  pinMode(RAM_DAT, INPUT);
  data = seRamReadData();
  pinMode(RAM_DAT, OUTPUT);

#if 0
  Serial.print(F("RAM mode:"));
  Serial.println(data, DEC);
#endif

  if (data == 0) {
    return;
  }

  //switch to byte mode
  // toggle the SHR CS to reset the bus for serial RAM
  digitalWrite(SHR_CS, 0);
  delayMicroseconds(CS_DELAY_US);
  digitalWrite(SHR_CS, 1);
  seRamWriteData(OPCODE_WRMR, 8); // 8 bits of Read Mode register
  seRamWriteData(0, 8); //write mode 0

}

static uint8_t seRamInit(void) {
  uint8_t r;

#if 0
  pinMode(SHR_CS, OUTPUT);
  pinMode(RAM_CLK, OUTPUT);
  pinMode(RAM_DAT, OUTPUT);
#endif

  seRamSetupMode();
  //try 16bit addressing mode (64kb RAM)
  ramAddrBits24 = 0;

  // detect SRAM presence by writing and reading data
  seRamWrite(0, 0x5A);
  r = seRamRead(0);

#if 0
  Serial.print("r:");
  Serial.println(r, DEC);
#endif


  if (r != 0x5A) {
    // try 24 bit addressing mode (128kb RAM)
    ramAddrBits24 = 1;
    seRamWrite(0, 0x5A);
    r = seRamRead(0);
    if (r != 0x5A) {
      return 0;
    }
  }

  seRamWrite(0xFFFF, 0xA5);
  r = seRamRead(0xFFFF);
  if (r != 0xA5) {
    return 0;
  }
  //verify the data at address 0 still exists
  r = seRamRead(0);
  return (r == 0x5A) ? (ramAddrBits24 + 1) : 0;
}

#endif /* RAM_BIG */

#endif /*_AFTB_SERAM_*/