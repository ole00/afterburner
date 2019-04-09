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
http://molej.cz/index_aft.html

Based on ATFblast 3.1 by Bruce Abbott
http://www.bhabbott.net.nz/atfblast.html

Based on GALBLAST by Manfred Winterhoff
http://www.armory.com/%7Erstevew/Public/Pgmrs/GAL/_ClikMe1st.htm

Supports:
* National GAL16V8
* Lattice GAL16V8A, GAL16V8B, GAL16V8D
* Lattice GAL22V10B
* Atmel ATF16V8B, ATF22V10B, ATF22V10CQZ

Requires:
* Arduino UNO with Afterburner sketch uploaded.
* simple programming circuit.

Changelog:
* 2019.02.02 - initial version 0.1
* 2019.03.24 - version 0.2
             - added support for Win32 and Win64 builds
             - fixed serial port setup for Mac OSX
* 2019.04.09 - version 0.3
             - fixed error detection
             - 'i' command now requires GAL type to be passed
               on the command line

This is the PC part that communicates with Arduino UNO by serial line.
To compile: gcc -g3 -O0 afterburner afterburner.c
*/


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "serial_port.h"

#define VERSION "v.0.3"


#define MAX_LINE 200

#define MAXFUSES 10000
#define GALBUFSIZE 16384

typedef enum {
    UNKNOWN,
    GAL16V8,
    GAL20V8,
    GAL22V10,
    ATF16V8B,
    ATF22V10B,
    ATF22V10C
} Galtype;


/* GAL info */
static struct {
    Galtype type;
    unsigned char id0, id1; /* variant 1, variant 2 (eg. 16V8=0x00, 16V8A+=0x1A)*/
    char *name;       /* pointer to chip name               */
    int fuses;        /* total number of fuses              */
    int pins;         /* number of pins on chip             */
    int rows;         /* number of fuse rows                */
    int bits;         /* number of fuses per row            */
    int uesrow;       /* UES row number                     */
    int uesfuse;      /* first UES fuse number              */
    int uesbytes;     /* number of UES bytes                */
    int eraserow;     /* row adddeess for erase             */
    int eraseallrow;  /* row address for erase all          */
    int pesrow;       /* row address for PES read/write     */
    int pesbytes;     /* number of PES bytes                */
    int cfgrow;       /* row address of config bits         */
    int cfgbits;      /* number of config bits              */
}
galinfo[] = {
    {UNKNOWN,   0x00, 0x00, "unknown",     0, 0, 0,  0, 0,   0, 0, 0, 0, 0, 8, 0, 0},
    {GAL16V8,   0x00, 0x1A, "GAL16V8",  2194, 20, 32, 64, 32, 2056, 8, 63, 54, 58, 8, 60, 82},
    {GAL20V8,   0x20, 0x3A, "GAL20V8",  2706, 24, 40, 64, 40, 2568, 8, 63, 59, 58, 8, 60, 82},
    {GAL22V10,  0x48, 0x49, "GAL22V10", 5892, 24, 44, 132, 44, 5828, 8, 61, 60, 58, 10, 16, 20},
    {ATF16V8B,  0x00, 0x00, "ATF16V8B", 2194, 20, 32, 64, 32, 2056, 8, 63, 54, 58, 8, 60, 82},
    {ATF22V10B, 0x00, 0x00, "ATF22V10B", 5892, 24, 44, 132, 44, 5828, 8, 61, 60, 58, 10, 16, 20},
    {ATF22V10C, 0x00, 0x00, "ATF22V10C", 5892, 24, 44, 132, 44, 5828, 8, 61, 60, 58, 10, 16, 20},
};

char verbose = 0;
char* filename = 0;
char* deviceName = 0;

SerialDeviceHandle serialF = INVALID_HANDLE;
Galtype gal;
int security = 0;
unsigned short checksum;
char galbuffer[GALBUFSIZE];
char fusemap[MAXFUSES];
char noGalCheck = 0;

char opRead = 0;
char opWrite = 0;
char opErase = 0;
char opInfo = 0;
char opVerify = 0;
char opTestVPP = 0;


static int waitForSerialPrompt(char* buf, int bufSize, int maxDelay);
static char sendGenericCommand(const char* command, const char* errorText, int maxDelay, char printResult);


static void printHelp() {
    printf("Afterburner " VERSION "  a GAL programming tool for Arduino based programmer\n");
    printf("more info: molej.cz/index_aft.html\n");
    printf("usage: afterburner command(s) [options]\n");
    printf("commands: ierwvs\n");
    printf("   i : read device info and programming voltage\n");
    printf("   r : read fuse map from the GAL chip and display it, -t option must be set\n");
    printf("   w : write fuse map, -f  and -t options must be set\n");
    printf("   v : verify fuse map, -f and -t options must be set\n");
    printf("   e : erase the GAL chip,  -t options must be set\n");
    printf("   s : sets VPP ON to check the programming voltage. Ensure the GAL is NOT inserted.\n");
    printf("options:\n");
    printf("  -v : verbose mode\n");
    printf("  -t <gal_type> : the GAL type. use GAL16V8 GAL20V8 GAL22V10 ATF16V8B ATF22V10B ATF22V10C\n");
    printf("  -f <file> : JEDEC fuse map file\n");
    printf("  -d <serial_device> : name of the serial device. Default is: %s\n", DEFAULT_SERIAL_DEVICE_NAME);
    printf("                       serial params are: 38400, 8N1\n");
    printf("  -nc : do not check device GAL type before operation: force the GAL type set on command line\n");
    printf("examples:\n");
    printf("  afterburner i -t ATF16V8B : reads and prints the device info\n");
    printf("  afterburner r -t ATF16V8B : reads the fuse map from the GAL chip and displays it\n");
    printf("  afterburner wv -f fuses.jed -t ATF16V8B : reads fuse map from file and writes it to \n");
    printf("              the GAL chip. Does the fuse map verification at the end.\n");
    printf("hints:\n");
    printf("  - use the 'i' command first to check and set the right programming voltage (VPP)\n");
    printf("         of the chip. If the programing voltage is unknown use 10V.\n");
    printf("  - known VPP voltages as tested on Afterburner with Arduino UNO: \n");
    printf("        Lattice GAL16V8D, GAL22V10D: 12V \n");
    printf("        Atmel   ATF16V8D, ATF22V10C: 10V \n");
}

static char checkArgs(int argc, char** argv) {
    int i;
    char* type = 0;
    char* modes = 0;

    gal = UNKNOWN;

    for (i = 1; i < argc; i++) {
        char* param = argv[i];
        if (strcmp("-t", param) == 0) {
            i++;
            type = argv[i];
        } else if (strcmp("-v", param) == 0) {
            verbose = 1;
        } else if (strcmp("-f", param) == 0) {
            i++;
            filename = argv[i];
        } else if (strcmp("-d", param) == 0) {
            i++;
            deviceName = argv[i];
        } else if (strcmp("-nc", param) == 0) {
            noGalCheck = 1;
        } else if (param[0] != '-') {
            modes = param;
        }
    }

    i = 0;
    while (modes != 0 && modes[i] != 0) {
        switch (modes[i]) {
        case 'r':
            opRead = 1;
            break;
        case 'w':
            opWrite = 1;
            break;
        case 'v':
            opVerify = 1;
            break;
        case 'e':
            opErase = 1;
            break;
        case 'i':
            opInfo = 1;
            break;
        case 's':
            opTestVPP = 1;
            break;

        default:
            printf("Error: unknown operation '%c' \n", modes[i]);
        }
        i++;
    }

    if (opRead == 0 && opWrite == 0 && opErase == 0 && opInfo == 0 && opVerify == 0 && opTestVPP == 0) {
        printHelp();
        printf("Error: no command specified.\n");
        return -1;
    }

    if (0 == filename && (opWrite == 1 || opVerify == 1)) {
        printf("Error: missing JED filename\n");
        return -1;
    }
    if (0 == type && (opWrite || opRead || opErase || opVerify || opInfo))  {
        printf("Error: missing GAL type. Use -t <type> to specify.\n");
        return -1;
    } else if (0 != type) {
        if (strcmp("GAL16V8", type) == 0) {
            gal = GAL16V8;
        }
        if (strcmp("GAL20V8", type) == 0) {
            gal = GAL20V8;
        }
        if (strcmp("GAL22V10", type) == 0) {
            gal = GAL22V10;
        }
        if (strcmp("ATF16V8B", type) == 0) {
            gal = ATF16V8B;
        }
        if (strcmp("ATF22V10B", type) == 0) {
            gal = ATF22V10B;
        }
        if (strcmp("ATF22V10C", type) == 0) {
            gal = ATF22V10C;
        }

        if (UNKNOWN == gal) {
            printf("Error: unknow GAL type. Types: GAL16V8 GAL20V8 GAL22V10 ATF16V8B ATF22V10B ATF22V10C\n");
            return -1;
        }
    }

    return 0;
}

static unsigned short checkSum(unsigned short n) {
    unsigned short c, e, i;
    unsigned long a;

    c = e = 0;
    a = 0;
    for (i = 0; i < n; i++) {
        e++;
        if (e == 9) {
            e = 1;
            a += c;
            c = 0;
        }
        c >>= 1;
        if (fusemap[i]) {
            c += 0x80;
        }
    }
    return (unsigned short)((c >> (8 - e)) + a);
}

static int parseFuseMap(char *ptr) {
    int i, n, type, checksumpos, address, pins, lastfuse;
    int state = 0; // 0=outside JEDEC, 1=skipping comment or unknown, 2=read command

    security = 0;
    checksum = 0;
    checksumpos = 0;
    pins = 0;
    lastfuse = 0;

    for (n = 0; ptr[n]; n++) {
        if (ptr[n] == '*') {
            state = 2;
        } else
            switch (state) {
            case 2:
                if (!isspace(ptr[n]))
                    switch (ptr[n]) {
                    case 'L':
                        address = 0;
                        state = 3;
                        break;
                    case 'F':
                        state = 5;
                        break;
                    case 'G':
                        state = 13;
                        break;
                    case 'Q':
                        state = 7;
                        break;
                    case 'C':
                        checksumpos = n;
                        state = 14;
                        break;
                    default:
                        state = 1;
                    }
                break;
            case 3:
                if (!isdigit(ptr[n])) {
                    return n;
                }
                address = ptr[n] - '0';
                state = 4;
                break;
            case 4:
                if (isspace(ptr[n])) {
                    state = 6;
                } else if (isdigit(ptr[n])) {
                    address = 10 * address + (ptr[n] - '0');
                } else {
                    return n;
                }
                break;
            case 5:
                if (isspace(ptr[n])) break; // ignored
                if (ptr[n] == '0' || ptr[n] == '1') {
                    memset(fusemap, ptr[n] - '0', sizeof(fusemap));
                } else {
                    return n;
                }
                state = 1;
                break;
            case 6:
                if (isspace(ptr[n])) break; // ignored
                if (ptr[n] == '0' || ptr[n] == '1') {
                    fusemap[address++] = ptr[n] - '0';
                } else {
                    return n;
                }
                break;
            case 7:
                if (isspace(ptr[n])) break; // ignored
                if (ptr[n] == 'P') {
                    pins = 0;
                    state = 8;
                } else if (ptr[n] == 'F') {
                    lastfuse = 0;
                    state = 9;
                } else {
                    state = 2;
                }
                break;
            case 8:
                if (isspace(ptr[n])) break; // ignored
                if (!isdigit(ptr[n])) return n;
                pins = ptr[n] - '0';
                state = 10;
                break;
            case 9:
                if (isspace(ptr[n])) break; // ignored
                if (!isdigit(ptr[n])) return n;
                lastfuse = ptr[n] - '0';
                state = 11;
                break;
            case 10:
                if (isdigit(ptr[n])) {
                    pins = 10 * pins + (ptr[n] - '0');
                } else if (isspace(ptr[n])) {
                    state = 12;
                } else {
                    return n;
                }
                break;
            case 11:
                if (isdigit(ptr[n])) {
                    lastfuse = 10 * lastfuse + (ptr[n] - '0');
                } else if (isspace(ptr[n])) {
                    state = 12;
                } else {
                    return n;
                }
                break;
            case 12:
                if (!isspace(ptr[n])) {
                    return n;
                }
                break;
            case 13:
                if (isspace(ptr[n])) break; // ignored
                if (ptr[n] == '0' || ptr[n] == '1') {
                    security = ptr[n] - '0';
                } else {
                    return n;
                }
                state = 1;
                break;
            case 14:
                if (isspace(ptr[n])) break; // ignored
                if (isdigit(ptr[n])) {
                    checksum = ptr[n] - '0';
                } else if (toupper(ptr[n]) >= 'A' && toupper(ptr[n]) <= 'F') {
                    checksum = toupper(ptr[n]) - 'A' + 10;
                } else return n;
                state = 15;
                break;
            case 15:
                if (isdigit(ptr[n])) {
                    checksum = 16 * checksum + ptr[n] - '0';
                } else if (toupper(ptr[n]) >= 'A' && toupper(ptr[n]) <= 'F') {
                    checksum = 16 * checksum + toupper(ptr[n]) - 'A' + 10;
                } else if (isspace(ptr[n])) {
                    state = 2;
                } else return n;
                break;
            }
    }

    if (lastfuse || pins) {
        int cs = checkSum(lastfuse);
        if (checksum && checksum != cs) {
            printf("Checksum does not match! given=0x%04X calculated=0x%04X last fuse=%i\n", checksum, cs, lastfuse);
        }

        for (type = 0, i = 1; i < sizeof(galinfo) / sizeof(galinfo[0]); i++) {
            if (
                (lastfuse == 0 ||
                 galinfo[i].fuses == lastfuse ||
                 galinfo[i].uesfuse == lastfuse && galinfo[i].uesfuse + 8 * galinfo[i].uesbytes == galinfo[i].fuses)
                &&
                (pins == 0 ||
                 galinfo[i].pins == pins ||
                 galinfo[i].pins == 24 && pins == 28)
            ) {
                if (gal == 0) {
                    type = i;
                    break;
                } else if (!type) {
                    type = i;
                }
            }
        }
    }
    return n;
}

static char readJedec(void) {
    FILE* f;
    int size;

    f = fopen(filename, "r");
    if (f) {
        size = fread(galbuffer, 1, GALBUFSIZE, f);
        fclose(f);
        galbuffer[size] = 0;
    } else {
        printf("Error: failed to open file: %s\n", filename);
        return -1;
    }
    return 0;
}

static int openSerial(void) {
    char buf[512];
    char devName[256] = {0};
    int total;
    int labelPos;


    //open device name
    snprintf(devName, sizeof(devName), "%s", (deviceName == 0) ? DEFAULT_SERIAL_DEVICE_NAME : deviceName);
    serialDeviceCheckName(devName, sizeof(devName));

    if (verbose) {
        printf("opening serial: %s\n", devName);
    }

    serialF = serialDeviceOpen(devName);
    if (serialF == INVALID_HANDLE) {
        printf("Error: failed to open serial device: %s\n", devName);
        return -2;
    }


    // prod the programmer to output it's identification
    sprintf(buf, "*\r");
    serialDeviceWrite(serialF, buf, 2);

    //read programmer's message
    total = waitForSerialPrompt(buf, 512, 3000);
    buf[total] = 0;

    //check we are communicating with Afterburner programmer
    labelPos = strstr(buf, "AFTerburner v.") -  buf;

    if (labelPos >= 0 && labelPos < 500 && buf[total - 3] == '>') {
        //all OK
        return 0;
    }
    if (verbose) {
        printf("Output from programmer not recognised: %s\n", buf);
    }
    serialDeviceClose(serialF);
    serialF = INVALID_HANDLE;
    return -4;
}

static void closeSerial(void) {
    if (INVALID_HANDLE == serialF) {
        return;
    }
    serialDeviceClose(serialF);
    serialF = INVALID_HANDLE;
}


static int checkPromptExists(char* buf, int bufSize) {
    int i;
    for (i = 0; i < bufSize && buf[i] != 0; i++) {
        if (buf[i] == '>') {
            return i;
        }
    }
    return -1;
}

static char* stripPrompt(char* buf) {
    int len;
    int i;
    if (buf == 0) {
        return 0;
    }
    len = strlen(buf);
    i  = checkPromptExists(buf, len);
    if (i >= 0) {
        buf[i] = 0;
        len = i;
    }

    //strp rear new line characters
    for (i = len - 1; i >= 0; i--) {
        if (buf[i] != '\r' && buf[i] != '\n') {
            break;
        } else {
            buf[i] = 0;
        }
    }

    //strip frontal new line characters
    for (i = 0; buf[i] != 0; i++) {
        if (buf[0] == '\r' || buf[0] == '\n') {
            buf++;
        }
    }
    return buf;
}

//finds beginnig of the last line
static char* findLastLine(char* buf) {
    int i;
    char* result = buf;

    if (buf == 0) {
        return 0;
    }
    for (i = 0; buf[i] != 0; i++) {
        if (buf[i] == '\r' || buf[i] == '\n') {
            result = buf + i + 1;
        }      
    }
    return result;
}

static int waitForSerialPrompt(char* buf, int bufSize, int maxDelay) {
    char* bufStart = buf;
    int bufTotal = bufSize;
    int bufPos = 0;
    int readSize;

    memset(buf, 0, bufSize);

    while (maxDelay > 0) {
        readSize = serialDeviceRead(serialF, buf, bufSize);
        if (readSize > 0) {
            bufPos += readSize;
            if (checkPromptExists(bufStart, bufTotal) >= 0) {
                maxDelay = 0; //force exit
            } else {
                buf += readSize;
                bufSize -= readSize;
            }
        }
        if (maxDelay > 0) {
            usleep(3 * 1000);
            maxDelay -= 3;
        }
    }
    return bufPos;
}

static int sendLine(char* buf, int bufSize, int maxDelay) {
    int total;
    int writeSize;
    char* obuf = buf;

    if (serialF == INVALID_HANDLE) {
        return -1;
    }

    if (buf == 0) {
        return -1;
    }

    total = strlen(buf);
    // write the query into the modem's file
    // file is opened non blocking so we have to ensure all contents is written
    while (total > 0) {
        writeSize = serialDeviceWrite(serialF, buf, total);
        if (writeSize < 0) {
            printf("ERROR: written: %i (%s)\n", writeSize, strerror(errno));
            return -4;
        }
        buf += writeSize;
        total -= writeSize;
    }

    total = waitForSerialPrompt(obuf, bufSize, (maxDelay < 0) ? 6 : maxDelay);
    obuf[total] = 0;
    obuf = stripPrompt(obuf);
    if (verbose) {
        printf("read: %i '%s'\n", total, obuf);
    }

    return total;
}

// Upload fusemap in byte format (as opposed to bit format used in JEDEC file).
static char upload() {
    char fuseSet;
    char buf[MAX_LINE];
    char line[64];
    unsigned int i, j, n;

    if (openSerial() != 0) {
        return -1;
    }

    // Start  upload
    sprintf(buf, "u\r");
    sendLine(buf, MAX_LINE, 20);

    //device type
    sprintf(buf, "#t %i %s\r", (int) gal, galinfo[gal].name);
    sendLine(buf, MAX_LINE, 300);

    //fuse map
    buf[0] = 0;
    fuseSet = 0;
    for (i = 0; i < galinfo[gal].fuses;) {
        unsigned char f = 0;
        if (i % 32 == 0) {
            if (i != 0) {
                strcat(buf, "\r");
                //the buffer contains at least one fuse set to 1
                if (fuseSet) {
#ifdef DEBUG_UPLOAD
                    printf("%s\n", buf);
#endif
                    sendLine(buf, MAX_LINE, 100);
                    buf[0] = 0;
                }
                fuseSet = 0;
            }
            sprintf(buf, "#f %04i ", i);
        }
        f = 0;
        for (j = 0; j < 8 && i < galinfo[gal].fuses; j++,i++) {
            if (fusemap[i]) {
                f |= (1 << j);
                fuseSet = 1;
            }
        }

        sprintf(line, "%02X", f);
        strcat(buf, line);
 
    }

    // send last unfinished fuse line
    if (i % 32 && fuseSet) {
        strcat(buf, "\r");
#ifdef DEBUG_UPLOAD
        printf("%s\n", buf);
#endif
        sendLine(buf, MAX_LINE, 100);
    }

    //checksum
    if (verbose) {
        printf("sending csum: %04X\n", checkSum(galinfo[gal].fuses));
    }
    sprintf(buf, "#c %04X\r", checkSum(galinfo[gal].fuses));
    sendLine(buf, MAX_LINE, 300);

    //end of upload
    return sendGenericCommand("#e\r", "Upload failed", 300, 0); 

}

static char sendGenericCommand(const char* command, const char* errorText, int maxDelay, char printResult) {
    char buf[MAX_LINE];
    int readSize;

    sprintf(buf, "%s", command);
    readSize = sendLine(buf, MAX_LINE, maxDelay);
    if (readSize < 0)  {
        if (verbose) {
            printf("%s\n", errorText);
        }
        return -1;
    } else {
        char* response = stripPrompt(buf);
        char* lastLine = findLastLine(response);
        if (lastLine == 0 || (lastLine[0] == 'E' && lastLine[1] == 'R')) {
            printf("%s\n", response);
            return -1;
        } else if (printResult) {
            printf("%s\n", response);
        }
    }
    return 0;
}

static char operationWriteOrVerify(char doWrite) {
    char buf[MAX_LINE];
    int readSize;

    char result;

    if (readJedec()) {
        return -1;
    }

    result = parseFuseMap(galbuffer);
    if (verbose) {
        printf("parse result=%i\n", result);
    }
    result = upload();
    if (result) {
        return result;
    }

    // write command
    if (doWrite) {
        result = sendGenericCommand("w\r", "write failed ?", 4000, 0);
        if (result) {
            goto finish;
        }
    }

    // verify command
    if (opVerify) {
        result = sendGenericCommand("v\r", "verify failed ?", 4000, 0);
    }
finish:
    closeSerial();
    return result;
}


static char operationReadInfo(void) {

    char result;

    if (openSerial() != 0) {
        return -1;
    }

    if (verbose) {
        printf("sending 'p' command...\n");
    }
    result = sendGenericCommand("p\r", "info failed ?", 4000, 1);

    closeSerial();
    return result;
}

// Test of programming voltage. Most chips require +12V to start prograaming.
// This test function turns ON the ENable pin so the Programming voltage is set.
// After 20 seconds the ENable pin is turned OFF. This gives you time to turn the
// pot on the MT3608 module and calibrate the right voltage for the GAL chip.
static char operationTestVpp(void) {

    char result;

    if (openSerial() != 0) {
        return -1;
    }

    if (verbose) {
        printf("sending 't' command...\n");
    }
    printf("Turn the Pot on the MT3608 module to check / set the VPP\n");
    //Voltage testing takes ~20 seconds
    result = sendGenericCommand("t\r", "info failed ?", 22000, 1);

    closeSerial();
    return result;
}

static char operationSetGalCheck(void) {
    int readSize;
    char result;

    if (openSerial() != 0) {
        return -1;
    }
    result = sendGenericCommand(noGalCheck ? "F\r" : "f\r", "noGalCheck failed ?", 4000, 0);
    closeSerial();
    return result;    
}

static char operationSetGalType(Galtype type) {
    char buf[MAX_LINE];
    int readSize;
    char result;

    if (openSerial() != 0) {
        return -1;
    }
    sprintf(buf, "g%i\r", (int)type); 
    result = sendGenericCommand(buf, "setGalType failed ?", 4000, 0);
    closeSerial();
    return result;    
}


static char operationEraseGal(void) {
    char buf[MAX_LINE];
    int readSize;
    char result;

    if (openSerial() != 0) {
        return -1;
    }

    //Switch to upload mode to specify GAL
    sprintf(buf, "u\r");
    sendLine(buf, MAX_LINE, 300);

    //set GAL type
    sprintf(buf, "#t %i\r", (int) gal);
    sendLine(buf, MAX_LINE, 300);

    //Exit upload mode (ensure the return texts are discarded by waiting 100 ms)
    sprintf(buf, "#e\r");
    sendLine(buf, MAX_LINE, 100);

    result = sendGenericCommand("c\r", "erase failed ?", 4000, 0);

    closeSerial();
    return result;
}

static char operationReadFuses(void) {
    char* response;
    char* buf = galbuffer;
    int readSize;

    if (openSerial() != 0) {
        return -1;
    }

    //Switch to upload mode to specify GAL
    sprintf(buf, "u\r");
    sendLine(buf, MAX_LINE, 100);

    //set GAL type
    sprintf(buf, "#t %i\r", (int) gal);
    sendLine(buf, MAX_LINE, 100);

    //Exit upload mode (ensure the texts are discarded by waiting 100 ms)
    sprintf(buf, "#e\r");
    sendLine(buf, MAX_LINE, 1000);

    //READ_FUSE command
    sprintf(buf, "r\r");
    readSize = sendLine(buf, GALBUFSIZE, 5000);
    if (readSize < 0)  {
        return -1;
    }
    response = stripPrompt(buf);
    printf("%s\n", response);

    closeSerial();

    if (response[0] == 'E' && response[1] == 'R') {
        return -1;
    }

    return 0;
}

int main(int argc, char** argv) {
    char result = 0;
    int i;

    result = checkArgs(argc, argv);
    if (result) {
        return result;
    }
    if (verbose) {
        printf("Afterburner " VERSION " \n");
    }

    result = operationSetGalCheck();

    if (gal != UNKNOWN && 0 == result) {
        result = operationSetGalType(gal);
    }

    if (opErase && 0 == result) {
        result = operationEraseGal();
    }

    if (0 == result) {
        if (opWrite) {
            // writing fuses and optionally verification
            result = operationWriteOrVerify(1);
        } else if (opInfo) {
            result = operationReadInfo();
        } else if (opRead) {
            result = operationReadFuses();
        } else if (opVerify) {
            // verification without writing
            result = operationWriteOrVerify(0);
        } else if (opTestVPP) {
            result = operationTestVpp();
        }
    }

    if (verbose) {
        printf("result=%i\n", (char)result);
    }
    return result;
}
