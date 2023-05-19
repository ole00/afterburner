//MCP4131 digital pot (bitbanged control) for Afterburner GAL project.
//  * compatible with MCP4151 (resolution of the wiper is halved to match MCP4131)

//set default pins
#ifndef POT_CS
#define POT_CS   A3
#endif

#ifndef POT_CLK
#define POT_CLK  A4
#endif

#ifndef POT_DAT
#define POT_DAT  A5
#endif

#ifndef POT_DEFAULT_VALUE
#define POT_DEFAULT_VALUE 0x40
#endif

#ifndef POT_WIPER_ENABLED
#define POT_WIPER_ENABLED 0
#endif

#define ADDR_WIPER 0
#define ADDR_WIPER0 0
#define ADDR_WIPER1 1
#define ADDR_TCON  4
#define ADDR_STAT  5
#define ADDR_INCREMENT 100
#define CMD_READ  (0b11 << 10)
#define CMD_INCREMENT (0b01 << 2)

#define mcp4131_disableWiper() mcp4131_write(ADDR_TCON, 0b111111101)
#define mcp4131_enableWiper() mcp4131_write(ADDR_TCON, 0b111111111)
#define mcp4131_read(A)  mcp4131_reg((A),0,1)
#define mcp4131_write(A,V)  mcp4131_reg((A),(V),0)

static uint8_t mcp4151_detected;

// read or write the mcp4131 register
static uint16_t  mcp4131_reg(uint8_t address, uint16_t value, uint8_t read_reg) {
    int8_t i = 15; //16 bit command
    uint16_t r = address;

    r <<= 12;

    if (mcp4151_detected && address == ADDR_WIPER) {
        value <<= 1; //multiply the wiper value by 2
    }

    if (address == ADDR_INCREMENT) {
        r = CMD_INCREMENT;
        i = 7; // 8 bit command
    } else
    if (read_reg) {
        r |= CMD_READ; 
    } else {
        r |= value & 0x1FF; // clamp value to 9 bits   
    }

    //setup Clock and and Data (SPI mode 0,0)
    digitalWrite(POT_CLK, 0);
    digitalWrite(POT_DAT, 0);
    delayMicroseconds(50);
    //activate IC
    digitalWrite(POT_CS, 0);

    while (i >= 0) {
        //write address and command (bits 15 to 10)       
        if ((!read_reg) || i > 9) {
            uint16_t mask =  (1 << i);
            digitalWrite(POT_DAT, (r & mask) ? 1 : 0);
        }
        digitalWrite(POT_CLK, 1); //rise the clock
        //only  when reading reg
        if (read_reg) {
            //switch the DAT pin to Input
            if (i == 10) {
                 pinMode(POT_DAT, INPUT);
            }
            //read bits 9 to 0
            if (i < 10) {
                r |= (digitalRead(POT_DAT) << i); //read after rising edge
            }
        }
        digitalWrite(POT_CLK, 0); //fall the clock
        i--;
    }
    if (read_reg) {
        pinMode(POT_DAT, OUTPUT);
    }

    if (mcp4151_detected && address == ADDR_WIPER && read_reg) {
        value >>= 1; //divide the wiper value by 2
    }

    //disable IC
    digitalWrite(POT_CS, 1);
    return r & 0x1FF; //clamp value to 9 bits
}

static void mcp4131_init(void) {   
    pinMode(POT_CS, OUTPUT);
    pinMode(POT_CLK, OUTPUT);
    pinMode(POT_DAT, OUTPUT);

    digitalWrite(POT_CS, 1); //unselect the POT's SPI bus
}

// initialisation and detection
// returns 1 if POT is detected, 0 otherwise
static uint8_t mcp4131_detect(void) {
    uint16_t r;

    mcp4151_detected = 0;

    mcp4131_disableWiper();

    //note checking is done while the wiper is disabled - no resistance is applied
    mcp4131_write(ADDR_WIPER, 0b1010);
    r = mcp4131_read(ADDR_WIPER);
    if (r != 0b1010) {
        return 0; 
    }
    mcp4131_write(ADDR_WIPER, 0b101);
    r = mcp4131_read(ADDR_WIPER);
    if (r != 0b101) {
        return 0; 
    }

    //sanity check whether the incrementing works
    mcp4131_write(ADDR_WIPER, 127); 
    mcp4131_write(ADDR_INCREMENT, 0); 
    r = mcp4131_read(ADDR_WIPER);
    if (r != 128) {
        return 0;
    }
    //detect MCP4151 by incrementing again - MCP4131 clamps the value to 128, MCP4151 increments to 129
    mcp4131_write(ADDR_INCREMENT, 0);
    r = mcp4131_read(ADDR_WIPER);
#if 0    
    Serial.print(F("MCP41541 detect: "));
    Serial.println(r == 129 ? 1 : 0, DEC);
#endif
    if (r == 129) {
        mcp4151_detected = 1;
    } else 
    if (r != 128) {
        return 0; //error - the value should be clamped to 128
    }

    mcp4131_write(ADDR_WIPER, POT_DEFAULT_VALUE);
#if POT_WIPER_ENABLED
    mcp4131_enableWiper();
#endif
    return 1;   
}

