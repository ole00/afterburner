#ifndef _SERIAL_PORT_H_
#define _SERIAL_PORT_H_


char guessedSerialDevice[512] = {0};

#ifdef _USE_WIN_API_

#include <windows.h>

#define SerialDeviceHandle HANDLE
#define DEFAULT_SERIAL_DEVICE_NAME "COM1"
#define INVALID_HANDLE  INVALID_HANDLE_VALUE

// ideas: https://stackoverflow.com/questions/1388871/how-do-i-get-a-list-of-available-serial-ports-in-win32
static void serialDeviceGuessName(char** deviceName) {
    char buf[64 * 1024] = {0};
    int size = QueryDosDevice(NULL, buf, 64 * 1024);
    int topComNum = 0;

    // buffer was filled in
    if (size > 0) {
        char* text = buf;
        int pos = 0;
        int start = 0;

        // search the received buffer for COM string
        while (pos < size) {
            int nameLen;
            start = pos;
            // find the string terminator
            while(pos < size && buf[pos] != 0) {
                pos++;
            }
            nameLen = pos - start;
            // COM port found
            if (nameLen >= 4 && nameLen <=6 && 0 == strncmp(text, "COM", 3) && text[3] >= '0' && text[3] <= '9') {
                int comNum = atoi(text + 3);
                if (comNum > topComNum) {
                    topComNum = comNum;
                    strcpy(guessedSerialDevice, text);
                }
            }
            pos++;
            text = &buf[pos];
        }
        
        // if we have found a COM port then pass it back as the result
        if (topComNum > 0) {
            *deviceName = guessedSerialDevice;
        }
    }
}

// https://www.xanthium.in/Serial-Port-Programming-using-Win32-API

static inline SerialDeviceHandle serialDeviceOpen(char* deviceName) {
    SerialDeviceHandle h;

    h = CreateFile(
        deviceName,                  //port name
        GENERIC_READ | GENERIC_WRITE, //Read/Write
        0,                            // No Sharing
        NULL,                         // No Security
        OPEN_EXISTING,// Open existing port only
        0,            // Non Overlapped I/O
        NULL);        // Null for Comm Devices

    if (h != INVALID_HANDLE) {
        BOOL result;
        COMMTIMEOUTS timeouts = { 0 };

        DCB dcbSerialParams = { 0 }; // Initializing DCB structure
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
        result = GetCommState(h, &dcbSerialParams);

        if (!result) {
            return INVALID_HANDLE;
        }

        dcbSerialParams.BaudRate = CBR_57600; // Setting BaudRate
        dcbSerialParams.ByteSize = 8;         // Setting ByteSize = 8
        dcbSerialParams.StopBits = ONESTOPBIT;// Setting StopBits = 1
        dcbSerialParams.Parity   = NOPARITY;  // Setting Parity = None
        dcbSerialParams.fOutxCtsFlow = FALSE;
        dcbSerialParams.fOutxDsrFlow = FALSE;
        dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
        dcbSerialParams.fOutX = FALSE;
        dcbSerialParams.fInX = FALSE;
        dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
        dcbSerialParams.fBinary = TRUE;
        

        result = SetCommState(h, &dcbSerialParams);
        if (!result) {
            return INVALID_HANDLE;
        }

        timeouts.ReadIntervalTimeout         = 30; // in milliseconds
        timeouts.ReadTotalTimeoutConstant    = 30; // in milliseconds
        timeouts.ReadTotalTimeoutMultiplier  = 10; // in milliseconds
        timeouts.WriteTotalTimeoutConstant   = 50; // in milliseconds
        timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds

        result = SetCommTimeouts(h, &timeouts);
        if (!result) {
            return INVALID_HANDLE;
        }
        //ensure no leftover bytes exist on the serial line
        result = PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

        return h;
    } else {
        return INVALID_HANDLE;
    }
  
}

void serialDeviceCheckName(char* name, int maxSize) {
    int nameLen = strlen(name);
    //convert comxx to COMxx
    if (strncmp(name, "com", 3) == 0 && (nameLen > 3 && name[3] >= '0' && name[3] <= '9')) {
        name[0] = 'C';
        name[1] = 'O';
        name[2] = 'M';
    }

    // convert COMxx and higher to \\\\.\\COMxx
    if (strncmp(name, "COM", 3) == 0 && (nameLen > 4 && name[3] >= '0' && name[3] <= '9')) {
        if (nameLen + 4 < maxSize) {
            int i;
            for (i = nameLen - 1; i >= 0; i--) {
                name[ i + 4] = name[i]; 
            }
            name[0] = '\\';
            name[1] = '\\';
            name[2] = '.';
            name[3] = '\\';
        }
    }
}

static inline void serialDeviceClose(SerialDeviceHandle deviceHandle) {
    CloseHandle(deviceHandle);
}

static inline int serialDeviceWrite(SerialDeviceHandle deviceHandle, char* buffer, int bytesToWrite) {
    DWORD written = 0;
    WriteFile(deviceHandle, buffer, bytesToWrite, &written, NULL);
    return (int) written;
}

static inline int serialDeviceRead(SerialDeviceHandle deviceHandle, char* buffer, int bytesToRead) {
    DWORD read = 0;
    ReadFile(deviceHandle, buffer, bytesToRead, &read, NULL);
    return (int) read;
}

#else

#include <ctype.h>
#include <fcntl.h>
#include <errno.h>

#include <termios.h>


#define SerialDeviceHandle int
#define DEFAULT_SERIAL_DEVICE_NAME "/dev/ttyUSB0"

#define INVALID_HANDLE -1

#ifdef NO_CLOSE
static SerialDeviceHandle serH = INVALID_HANDLE;
#endif


#ifdef _OSX_
    #define CHECK_SERIAL() (text != NULL)
    #define LIST_DEVICES  "ls -1 /dev/tty.usb* /dev/tty.wchusb* 2>/dev/null"
#else    
    #define CHECK_SERIAL() (text != NULL && 0 == strncmp(text + 18, "usb-", 4))
    #define LIST_DEVICES "ls -1 /dev/serial/by-id/* 2>/dev/null"
#endif    

// ideas: https://unix.stackexchange.com/questions/647235/debian-how-to-identify-usb-devices-with-similar-dev-tty-file
//        https://pubs.opengroup.org/onlinepubs/009696799/functions/popen.html
static void serialDeviceGuessName(char** deviceName) {
    int i;
    char text[512];
    int topTtyNum = 0;

    FILE* f = popen(LIST_DEVICES, "r");
    if (f == NULL) {
        return;
    }
    i = 1;
    while (fgets(text, 512, f) != NULL) {
        // filer out non USB devices and prioritise those with Arduino name
        if (CHECK_SERIAL()) {
            int ttyNum  = i;
            int textLen = strlen(text);
            // prefer Arduino over generic USB serial ports (does not work on OSX)
            if (textLen > 29 && 0 == strncmp(text + 18, "usb-Arduino", 11)) {
                ttyNum += 1000;
            }
            if (ttyNum > topTtyNum) {
                topTtyNum = ttyNum;
                strcpy(guessedSerialDevice, text);
                //strip the new-line trailing markers
                if (guessedSerialDevice[textLen - 1] == '\n' || guessedSerialDevice[textLen - 1] == '\r') {
                    guessedSerialDevice[textLen - 1] = 0;
                }
            }
        }
        i++;
    }
    pclose(f);

    if (topTtyNum > 0) {
        *deviceName = guessedSerialDevice;
    }
}

static inline SerialDeviceHandle serialDeviceOpen(char* deviceName) {

    SerialDeviceHandle h;
#ifdef NO_CLOSE
    if (serH != INVALID_HANDLE) {
        return serH;
    }
#endif
    
    h = open(deviceName, O_RDWR | O_NOCTTY  | O_NONBLOCK);

    if (h != INVALID_HANDLE) {
        //set the serial port parameters
        struct termios serial;

        memset(&serial, 0, sizeof(struct termios));
        cfsetispeed(&serial, B57600);
        cfsetospeed(&serial, B57600);

        serial.c_cflag |= CS8; // no parity, 1 stop bit
        serial.c_cflag |= CREAD | CLOCAL;

        if (0 != tcsetattr(h, TCSANOW, &serial)) {
            close(h);
            printf("Error: failed to set serial parameters %i, %s\n", errno, strerror(errno));
            return INVALID_HANDLE;
        }

        //ensure no leftover bytes exist on the serial line
        tcdrain(h);
        tcflush(h, TCIOFLUSH); //flush both queues
#ifdef NO_CLOSE
        serH = h;
#endif
        return h;
    } else {
        return INVALID_HANDLE;
    }
}

void serialDeviceCheckName(char* name, int maxSize) {
    //nothing to check    
}

static inline void serialDeviceClose(SerialDeviceHandle deviceHandle) {
#ifndef NO_CLOSE
    close(deviceHandle);
#endif
}

static inline int serialDeviceWrite(SerialDeviceHandle deviceHandle, char* buffer, int bytesToWrite) {
    return write(deviceHandle, buffer, bytesToWrite);
}

static inline int serialDeviceRead(SerialDeviceHandle deviceHandle, char* buffer, int bytesToRead) {
    return read(deviceHandle, buffer, bytesToRead);
}
#endif

#endif /* _SERIAL_PORT_H_ */

