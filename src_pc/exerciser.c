/*

 EXERCISER : a script runner that exercises IC pins
 
 part of Afterburner GAL project

*/


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "exerciser.h"

#define DELAY_NONE 0
#define DELAY_DEFAULT 1
#define DELAY_CUSTOM 2

char sendGenericCommand(const char* command, const char* errorText, int maxDelay, char printResult);

typedef int (*ExeFunc)(char* line, int lineSize, int check);

static int execPinCount(char* line, int lineSize, int check);
static int execDefaultDelay(char* line, int lineSize, int check);
static int execEcho(char* line, int lineSize, int check);
static int execTest(char* line, int lineSize, int check);
static int execDelay(char* line, int lineSize, int check);
static int execPulseDuration(char* line, int lineSize, int check);
static int execTraceOn(char* line, int lineSize, int check);
static int execTraceOff(char* line, int lineSize, int check);
static int execQuit(char* line, int lineSize, int check);


typedef struct {
    const char* name;
    ExeFunc exeFunc;
    char doDelay;
} ExeCommand;


typedef struct {
    int lineNumber;
    char line[1024];
    int pinCount;
    int defaultDelay;
    int doDelay;
    int pulseDuration;
    int shrVal; // shift register value
    char shrZ; // shift register in Z state
    char verbose;
    char isPulsedCommand;
    char isTracing;
    char quit;
} ExeContext;


static const ExeCommand commands[] = {
    { "PinCount", execPinCount, DELAY_NONE },
    { "DefaultDelay", execDefaultDelay, DELAY_NONE },
    { "Echo", execEcho, DELAY_NONE },
    { "Test", execTest, DELAY_DEFAULT },
    { "Delay", execDelay, DELAY_CUSTOM },
    { "PulseDuration", execPulseDuration, DELAY_NONE },
    { "TraceOn", execTraceOn, DELAY_NONE },
    { "TraceOff", execTraceOff, DELAY_NONE },
    { "Quit", execQuit, DELAY_NONE },

    { "", NULL, 0 } //Terminator
};


static ExeContext exeCtx;

static char* skipFrontWhiteSpace(char* line, char* scriptEnd) {
    while (line < scriptEnd) {
        if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n' || line[0] == '\r') {
            line++;
        } else {
            break;
        }
    }
    return line;
}

static int getLineLength(char* line, char* scriptEnd) {
    int len = 0;
    while (line < scriptEnd) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == 0) {
            break;
        }
        len++;
        line++;
    }
    return len;
}

static void copyLineToCtx(char* line, int lineSize) {
    strncpy(exeCtx.line, line, lineSize > 1023 ? 1023 : lineSize);
    exeCtx.line[lineSize > 1023 ? 1023 : lineSize] = 0;
}


static void waitForKey(void) {
    char buf[512] = {0};

    printf(" ... Trace is On: press Enter to continue (x to quit)...\n");
    fgets(buf, sizeof(buf), stdin);
    if (
        0 == strncmp(buf, "quit\n", 5) ||
        0 == strncmp(buf, "quit\r", 5) ||
        0 == strncmp(buf, "exit\n", 5) ||
        0 == strncmp(buf, "exit\r", 5) ||
        0 == strncmp(buf, "x\n", 2) ||
        0 == strncmp(buf, "x\r", 2)
    ) {
        exeCtx.quit = 1;
        printf(" Quit\n");
    }
}

static int execTraceOn(char* line, int lineSize, int check) {
    exeCtx.isTracing = 1;
    return 0;
}

static int execTraceOff(char* line, int lineSize, int check) {
    exeCtx.isTracing = 0;
    return 0;
}

static int execQuit(char* line, int lineSize, int check) {
    exeCtx.quit = 1;
    if (!check) {
        printf(" Quit command\n");
    }
    return 0;
}

static int execPulseDuration(char* line, int lineSize, int check) {
    int duration = 100;

    copyLineToCtx(line, lineSize);
    sscanf(exeCtx.line, "%d", &duration);
    if (duration < 0 || duration >= 10000) {
        printf("Error: invalid PulseDuration (max is 9999) %d\n", duration);
        return -1;
    }
    exeCtx.pulseDuration = duration;

    if (check) {
        return 0;
    }

    if (exeCtx.verbose) {
        printf(" * %s %d\n", __FUNCTION__, duration);
    }
    return 0;
}

static int execPinCount(char* line, int lineSize, int check) {
    int count = -1;

    copyLineToCtx(line, lineSize);
    sscanf(exeCtx.line, "%d", &count);
    if (count != 20 && count != 24) {
        printf("Error: PinCount can be 20 or 24, not %d\n", count);
        return -1;
    }
    exeCtx.pinCount = count;

    if (check) {
        return 0;
    }

    if (exeCtx.verbose) {
        printf(" * %s %d\n", __FUNCTION__, count);
    }
    return 0;
}

static int execDefaultDelay(char* line, int lineSize, int check) {
    int delay = -1;
    copyLineToCtx(line, lineSize);
    sscanf(exeCtx.line, "%d", &delay);
    if (delay < 0 || delay > 1000000) {
        printf("Error: invalid default delay!\n");
        return -1;
    }
    if (check) {
        return 0;
    }

    if (exeCtx.verbose) {
        printf(" * %s %d\n", __FUNCTION__, delay);
    }
    exeCtx.defaultDelay = delay;
    return 0;
}

static int execDelay(char* line, int lineSize, int check) {
    int delay = -1;
    copyLineToCtx(line, lineSize);
    sscanf(exeCtx.line, "%d", &delay);
    if (delay < 0 || delay > 1000000) {
        printf("Error: invalid delay!\n");
        return -1;
    }

    if (check) {
        return 0;
    }

    if (exeCtx.verbose) {
        printf(" * %s %d\n", __FUNCTION__, delay);
    }


    if (exeCtx.isTracing) {
        waitForKey();
    }

    if (exeCtx.isPulsedCommand) {
        delay += (exeCtx.pulseDuration * 2) + 1;
    }

    usleep(delay * 1000);
    exeCtx.doDelay = 0; // don't do default delay for the next command
    return 0;
}


static int execEcho(char* line, int lineSize, int check) {
    if (check) {
        return 0;
    }
    copyLineToCtx(line, lineSize);

    printf("%s\n", exeCtx.line);
    //printf("%s\n", __FUNCTION__);
    return 0;
}

static char* getNextPin(char* pin, char* line) {
    while (*line != 0) {
        //skip white space
        if (*line == ' ' || *line == '_') {
            line++;
        } else {
            if (
                *line == '0' || *line == '1' || // Low / High  output
                *line == 'G' || *line == 'g' || // GND
                *line == 'V' || *line == 'v' || // VCC
                *line == 'Z' || *line == 'z' || // Input / High impedance
                *line == 'P' || *line == 'p' || // Rising pulse / Falling pulse
                *line == 'X' || *line == 'x'    // Do not care - leave the old setting
            ) {
                //convert to lower case except the pulse 'P' and 'p'
                if (*line == 'G' || *line == 'V' || *line == 'Z' || *line == 'X') {
                    *pin = *line + 32;
                } else {
                    *pin = *line;
                }
                line++;
                return line;
            } else {
                if (exeCtx.verbose) {
                    printf("Error: unknown character: %c\n", *line);
                }
                // unknown character detected
                return NULL;
            }
        }
    }
    if (exeCtx.verbose) {
        printf("Error: test pattern is too short\n");
    }
    return NULL;
}

static int setPins(char* pins) {
    int result;
    char tmp[64];

    sprintf(tmp, "x%s\r", pins);
    result = sendGenericCommand(tmp, "Excersize set pins failed?", 2000, 0);
    return result;
}

static int execTest(char* line, int lineSize, int check) {
    char pins[25];
    int i = 0;
    int gndIndex = 11;
    int vccIndex = 23;
    int cnt;

    exeCtx.isPulsedCommand = 0;

    copyLineToCtx(line, lineSize);

    //parse the line and put the pattern into the 'pins' array
    line = exeCtx.line;
    while (i < exeCtx.pinCount) {
        line = getNextPin(pins + i, line);
        if (NULL == line) {
            printf("Error: invalid test pattern: '%s' index=%d / %d \n", exeCtx.line, i, exeCtx.pinCount);
            return 1;
        }
        i++;
    }
    pins[exeCtx.pinCount] = 0;

    // sanity check: Voltage and GND pins are in the correct positions in the pattern
    if (20 == exeCtx.pinCount) {
        gndIndex = 9;
        vccIndex = 19;
    }

    if (pins[gndIndex] != 'g' || pins[vccIndex] != 'v') {
        printf("Error: invalid GND / VCC '%s' '%s' %c %c \n", exeCtx.line, pins, pins[gndIndex], pins[vccIndex]);
        return -1;
    }

    // check that pins 2 to 9 are all either Z or none is Z - these are controlled by the shift register
    cnt = 0;
    i = 2;
    while (i <= 9) {
        if (pins[i] == 'z' || pins[i] == 'x') {
            cnt++;
        }
        i++;
    }
    if (cnt != 0 && cnt != 8) {
        printf("Error: invalid Z state of pins 2-9 '%s '\n", pins);
        return 1;
    }
    // cnt is either 0 or 8
    exeCtx.shrZ = cnt / 8;

    // TODO check that pins 20-22 are either 'z' or 'x' as these can't be set

    // check whether there are pulsed pins
    i = 0;
    while (i < exeCtx.pinCount) {
        if (pins[i] == 'p' || pins[i] == 'P') {
            exeCtx.isPulsedCommand = 1;
            break;
        }
        i++;
    }

    if (check) {
        return 0;
    }

    if (exeCtx.verbose) {
        printf(" * %s %s\n", __FUNCTION__, pins);
    }

    return setPins(pins);
}

static int runBuffer(char* buffer, int bufSize, int checkSyntax) {
    char* line = buffer;
    char* scriptEnd = buffer + bufSize;
    int lineCount = 0;
    int errors = 0;

    exeCtx.quit = 0;
    exeCtx.shrVal = 0;
    exeCtx.pulseDuration = 100;
    exeCtx.lineNumber = 0;
    exeCtx.defaultDelay = 1000;
    exeCtx.pinCount = 24;
    exeCtx.doDelay = 0; // don't do default delay for the first test
    exeCtx.isTracing = 0;

    while (NULL != line && line < scriptEnd) {
        lineCount++;
        line = skipFrontWhiteSpace(line, scriptEnd);
        if (NULL != line) {
            int len = getLineLength(line, scriptEnd);
            if (len > 0) {
                int i = 0;
                int found = 0;

                // ignore comments
                if (line[0] != '#') {
                    while (1) {
                        const ExeCommand* cmd = &commands[i];
                        int maxLen = strlen(cmd->name);
                        if (NULL == cmd->exeFunc) {
                            break;
                        }
                        i++;
                        if (len < maxLen) {
                            continue;
                        }
                        if (0 == strncmp(cmd->name, line, maxLen)) {
                            found = 1;

                            // delay between commands if not checking syntax
                            if (!checkSyntax) {
                                // current command is not Delay command AND previous command asked to do default delay
                                if (cmd->doDelay != DELAY_CUSTOM  && exeCtx.doDelay == DELAY_DEFAULT) {
                                    int delayTime = exeCtx.defaultDelay;

                                    if (exeCtx.isTracing) {
                                        waitForKey();
                                    }

                                    if (exeCtx.isPulsedCommand) {
                                        delayTime += (exeCtx.pulseDuration * 2) + 1;
                                    }
                                    usleep(delayTime * 1000);
                                }
                            }

                            if (!exeCtx.quit) {
                                if (cmd->exeFunc(line + maxLen, len - maxLen, checkSyntax)) {
                                    errors++;
                                }
                                exeCtx.doDelay = cmd->doDelay;
                            }
                            break;
                        }
                    }
                    if (!found) {
                        copyLineToCtx(line, len);
                        printf("Error: unknown command '%s'\n", exeCtx.line);
                        errors++;
                    }
                    //printf("--------------------------\n");
                }
                line += len;
                if (!checkSyntax) {
                    if (exeCtx.quit) {
                        break;
                    }
                }

            } else {
                break;
            }
        } else {
            break;
        }
    }
    return errors;
}



int exerciseFile(char* buffer, int bufSize, int checkSyntax) {
    int result;
    if (NULL == buffer || bufSize < 1) {
        printf("[%s] Error: invalid prameters\n", __FUNCTION__);
        return 1;
    }
    // check the syntax only
    return runBuffer(buffer, bufSize, checkSyntax);
}

void exerciseSetVerbose(char verbose) {
    exeCtx.verbose = verbose ? 1 : 0;
}

int exerciseGetPinCount(void) {
    return exeCtx.pinCount;
}

int exerciseGetPulseDuration(void) {
    return exeCtx.pulseDuration;
}

