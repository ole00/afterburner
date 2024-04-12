#ifndef _JTAG_XSVF_PLAYER_H_
#define _JTAG_XSVF_PLAYER_H_


/*
Arduino JTAG Player for Afterburner GAL project
---------------------------------------
Adapted from JTAG library 1.0.15 by Marcelo Jimenez
https://github.com/mrjimenez/JTAG

This port:
* improves flash size on AVR MCU (about 6.5kb vs 11kb)

* allows to allocate JTAG internal buffers temporarily within a shared
  global buffer (heap). Define XSVF_HEAP to enable such feature:
  uint8_t heap[900];
  #define XSVF_HEAP heap
  #include "jtag_xsvf_player.h"

* reduces the code to a single .h file

Use the original JTAG libray python scripts to upload XSVF files
from your PC:
./xsvf -p /dev/ttyACM0 my_file.xsvf

Arduino usage:
  jtag_port_t jport;
  
  Serial.begin(115200);

  //assign jtag pins (vref pin checks the cable is plugged in)
  jport.tms = 12;
  jport.tdi = 2;
  jport.tdo = 4;
  jport.tck = 3;
  jport.vref = 10;

  //process XSVF data received from serial port
  jtag_play_xsvf(&jport);

*/

//value bigger than 63 may cause reading errors on AVR MCUs.
#define XSVF_BUF_SIZE 62

#define XSVF_DEBUG 0
#define XSVF_CALC_CSUM 1

#define		XCOMPLETE 0
#define		XTDOMASK 1
#define		XSIR 2
#define		XSDR 3
#define		XRUNTEST 4
#define		XRESERVED_5 5
#define		XRESERVED_6 6
#define		XREPEAT 7
#define		XSDRSIZE 8
#define		XSDRTDO 9
#define		XSETSDRMASKS 10
#define		XSDRINC 11
#define		XSDRB 12
#define		XSDRC 13
#define		XSDRE 14
#define		XSDRTDOB 15
#define		XSDRTDOC 16
#define		XSDRTDOE 17
#define		XSTATE 18
#define		XENDIR 19
#define		XENDDR 20
#define		XSIR2 21
#define		XCOMMENT 22
#define		XWAIT 23
#define		XWAITSTATE 24
#define		XTRST 28

#define S_MAX_CHAIN_SIZE_BYTES 129
#define S_MAX_CHAIN_SIZE_BITS (S_MAX_CHAIN_SIZE_BYTES * 8)

#define STATE_RUN_TEST_IDLE 1
#define STATE_PAUSE_DR 6
#define STATE_PAUSE_IR 13

#define ERR_IO 1
#define ERR_XSIR_SIZE 2
#define ERR_XSDRSIZE 3
#define ERR_XENDIR 4
#define ERR_XENDDR 5
#define ERR_XSDR 6
#define ERR_INSTR_NOT_IMPLEMENTED 99
#define ERR_DR_CHECK_FAILED 101


/*
 * Low nibble : TMS == 0
 * High nibble: TMS == 1
 */

#define TMS_T(TMS_HIGH_STATE, TMS_LOW_STATE) (((TMS_HIGH_STATE) << 4) | (TMS_LOW_STATE))

#define	XSTATE_TEST_LOGIC_RESET 0
#define	XSTATE_RUN_TEST_IDLE 1
#define	XSTATE_SELECT_DR_SCAN 2
#define	XSTATE_CAPTURE_DR 3
#define	XSTATE_SHIFT_DR 4
#define	XSTATE_EXIT1_DR 5
#define	XSTATE_PAUSE_DR 6
#define	XSTATE_EXIT2_DR 7
#define	XSTATE_UPDATE_DR 8
#define	XSTATE_SELECT_IR_SCAN 9 
#define	XSTATE_CAPTURE_IR 10
#define	XSTATE_SHIFT_IR 11
#define	XSTATE_EXIT1_IR 12
#define	XSTATE_PAUSE_IR 13
#define	XSTATE_EXIT2_IR 14
#define	XSTATE_UPDATE_IR 15

#define TMS_T00	  /* STATE_TEST_LOGIC_RESET */ TMS_T(XSTATE_TEST_LOGIC_RESET, XSTATE_RUN_TEST_IDLE)
#define TMS_T01		/* STATE_RUN_TEST_IDLE    */ TMS_T(XSTATE_SELECT_DR_SCAN,   XSTATE_RUN_TEST_IDLE)
#define TMS_T02		/* STATE_SELECT_DR_SCAN   */ TMS_T(XSTATE_SELECT_IR_SCAN,   XSTATE_CAPTURE_DR)
#define TMS_T03		/* STATE_CAPTURE_DR       */ TMS_T(XSTATE_EXIT1_DR,         XSTATE_SHIFT_DR)
#define TMS_T04		/* STATE_SHIFT_DR         */ TMS_T(XSTATE_EXIT1_DR,         XSTATE_SHIFT_DR)
#define TMS_T05		/* STATE_EXIT1_DR         */ TMS_T(XSTATE_UPDATE_DR,        XSTATE_PAUSE_DR)
#define TMS_T06		/* STATE_PAUSE_DR         */ TMS_T(XSTATE_EXIT2_DR,         XSTATE_PAUSE_DR)
#define TMS_T07		/* STATE_EXIT2_DR         */ TMS_T(XSTATE_UPDATE_DR,        XSTATE_SHIFT_DR)
#define TMS_T08		/* STATE_UPDATE_DR        */ TMS_T(XSTATE_SELECT_DR_SCAN,   XSTATE_RUN_TEST_IDLE)
#define TMS_T09		/* STATE_SELECT_IR_SCAN   */ TMS_T(XSTATE_TEST_LOGIC_RESET, XSTATE_CAPTURE_IR)
#define TMS_T10		/* STATE_CAPTURE_IR       */ TMS_T(XSTATE_EXIT1_IR,         XSTATE_SHIFT_IR)
#define TMS_T11		/* STATE_SHIFT_IR         */ TMS_T(XSTATE_EXIT1_IR,         XSTATE_SHIFT_IR)
#define TMS_T12		/* STATE_EXIT1_IR         */ TMS_T(XSTATE_UPDATE_IR,        XSTATE_PAUSE_IR)
#define TMS_T13		/* STATE_PAUSE_IR         */ TMS_T(XSTATE_EXIT2_IR,         XSTATE_PAUSE_IR)
#define TMS_T14		/* STATE_EXIT2_IR         */ TMS_T(XSTATE_UPDATE_IR,        XSTATE_SHIFT_IR)
#define TMS_T15		/* STATE_UPDATE_IR        */ TMS_T(XSTATE_SELECT_DR_SCAN,   XSTATE_RUN_TEST_IDLE)


#define BITSTR(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P) ( \
	((uint16_t)(A) << 15) | \
	((uint16_t)(B) << 14) | \
	((uint16_t)(C) << 13) | \
	((uint16_t)(D) << 12) | \
	((uint16_t)(E) << 11) | \
	((uint16_t)(F) << 10) | \
	((uint16_t)(G) <<  9) | \
	((uint16_t)(H) <<  8) | \
	((uint16_t)(I) <<  7) | \
	((uint16_t)(J) <<  6) | \
	((uint16_t)(K) <<  5) | \
	((uint16_t)(L) <<  4) | \
	((uint16_t)(M) <<  3) | \
	((uint16_t)(N) <<  2) | \
	((uint16_t)(O) <<  1) | \
	((uint16_t)(P) <<  0) )

/*
 * The index of this vector is the current state. The i-th bit tells you the
 * value TMS must assume in order to go to state "i".

------------------------------------------------------------------------------------------------------------
|                        |   || F | E | D | C || B | A | 9 | 8 || 7 | 6 | 5 | 4 || 3 | 2 | 1 | 0 ||   HEX  |
------------------------------------------------------------------------------------------------------------
| STATE_TEST_LOGIC_RESET | 0 || 0 | 0 | 0 | 0 || 0 | 0 | 0 | 0 || 0 | 0 | 0 | 0 || 0 | 0 | 0 | 1 || 0x0001 |
| STATE_RUN_TEST_IDLE    | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 0 | 1 || 0xFFFD |
| STATE_SELECT_DR_SCAN   | 2 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 0 || 0 | 0 | 0 | 0 || 0 | x | 1 | 1 || 0xFE03 |
| STATE_CAPTURE_DR       | 3 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 0 || x | 1 | 1 | 1 || 0xFFE7 |
| STATE_SHIFT_DR         | 4 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 0 || 1 | 1 | 1 | 1 || 0xFFEF |
| STATE_EXIT1_DR         | 5 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 0 | 0 | x | 0 || 1 | 1 | 1 | 1 || 0xFF0F |
| STATE_PAUSE_DR         | 6 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 0 | 1 | 1 || 1 | 1 | 1 | 1 || 0xFFBF |
| STATE_EXIT2_DR         | 7 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || x | 0 | 0 | 0 || 1 | 1 | 1 | 1 || 0xFF0F |
| STATE_UPDATE_DR        | 8 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | x || 1 | 1 | 1 | 1 || 1 | 1 | 0 | 1 || 0xFEFD |
| STATE_SELECT_IR_SCAN   | 9 || 0 | 0 | 0 | 0 || 0 | 0 | x | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 0x01FF |
| STATE_CAPTURE_IR       | A || 1 | 1 | 1 | 1 || 0 | x | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 0xF3FF |
| STATE_SHIFT_IR         | B || 1 | 1 | 1 | 1 || 0 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 0xF7FF |
| STATE_EXIT1_IR         | C || 1 | 0 | 0 | x || 0 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 0x87FF |
| STATE_PAUSE_IR         | D || 1 | 1 | 0 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 0xDFFF |
| STATE_EXIT2_IR         | E || 1 | x | 0 | 0 || 0 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 0x87FF |
| STATE_UPDATE_IR        | F || x | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 1 | 1 || 1 | 1 | 0 | 1 || 0x7FFD |
------------------------------------------------------------------------------------------------------------

*/

#define BS00 /* STATE_TEST_LOGIC_RESET */ BITSTR(  0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 1  )
#define BS01 /* STATE_RUN_TEST_IDLE    */ BITSTR(  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 0, 1  )
#define BS02 /* STATE_SELECT_DR_SCAN   */ BITSTR(  1, 1, 1, 1,   1, 1, 1, 0,   0, 0, 0, 0,   0, 0, 1, 1  )
#define BS03 /* STATE_CAPTURE_DR       */ BITSTR(  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 0,   0, 1, 1, 1  )
#define BS04 /* STATE_SHIFT_DR         */ BITSTR(  1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1  )
#define BS05 /* STATE_EXIT1_DR         */ BITSTR(  1, 1, 1, 1,   1, 1, 1, 1,   0, 0, 0, 0,   1, 1, 1, 1  )
#define BS06 /* STATE_PAUSE_DR         */ BITSTR(  1, 1, 1, 1,   1, 1, 1, 1,   1, 0, 1, 1,   1, 1, 1, 1  )
#define BS07 /* STATE_EXIT2_DR         */ BITSTR(  1, 1, 1, 1,   1, 1, 1, 1,   0, 0, 0, 0,   1, 1, 1, 1  )
#define BS08 /* STATE_UPDATE_DR        */ BITSTR(  1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 0, 1  )
#define BS09 /* STATE_SELECT_IR_SCAN   */ BITSTR(  0, 0, 0, 0,   0, 0, 0, 1,   1, 1, 1, 1,   1, 1, 1, 1  )
#define BS10 /* STATE_CAPTURE_IR       */ BITSTR(  1, 1, 1, 1,   0, 0, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1  )
#define BS11 /* STATE_SHIFT_IR         */ BITSTR(  1, 1, 1, 1,   0, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1  )
#define BS12 /* STATE_EXIT1_IR         */ BITSTR(  1, 0, 0, 0,   0, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1  )
#define BS13 /* STATE_PAUSE_IR         */ BITSTR(  1, 1, 0, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1  )
#define BS14 /* STATE_EXIT2_IR         */ BITSTR(  1, 0, 0, 0,   0, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1  )
#define BS15 /* STATE_UPDATE_IR        */ BITSTR(  0, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 0, 1  )


typedef struct xsvf_t {
  uint8_t* xsvf_tdo_mask;
  uint8_t* xsvf_tdi;
  uint8_t* xsvf_tdo;
  uint8_t* xsvf_tdo_expected;
  uint8_t* xsvf_address_mask;
  uint8_t* xsvf_data_mask;

  uint32_t rdpos;
  uint32_t wrpos;

  #if XSVF_CALC_CSUM
  uint32_t csum;
  #endif

  uint16_t instruction_counter;
  uint8_t  error;
  uint8_t  xcomplete;

  uint16_t sirsize_bits;
  uint16_t sirsize_bytes;

  uint32_t sdrsize_bits;
  uint32_t sdrsize_bytes;
  uint32_t runtest;

  uint8_t  repeat;
  uint8_t  next_state;
  uint8_t  endir_state;
  uint8_t  enddr_state;

  uint32_t wait_time_usecs;
  uint8_t  wait_start_state;
  uint8_t  wait_end_state;
  uint8_t  jtag_current_state;

} xsvf_t;

#ifdef XSVF_HEAP
// variables will be allocated on heap
uint8_t* xsvf_buf;
xsvf_t* xsvf;
uint8_t* xsvf_tms_transitions;
uint16_t* xsvf_tms_map;
#else /* XSVF_HEAP */
// variables allocated globally
uint8_t xsvf_buf[XSVF_BUF_SIZE];
uint8_t xsvf_tdo_mask[S_MAX_CHAIN_SIZE_BYTES];
uint8_t xsvf_tdi[S_MAX_CHAIN_SIZE_BYTES];
uint8_t xsvf_tdo[S_MAX_CHAIN_SIZE_BYTES];
uint8_t xsvf_tdo_expected[S_MAX_CHAIN_SIZE_BYTES];
uint8_t xsvf_address_mask[S_MAX_CHAIN_SIZE_BYTES];
uint8_t xsvf_data_mask[S_MAX_CHAIN_SIZE_BYTES];
xsvf_t xsvf_context;
xsvf_t* xsvf = &xsvf_context;

static const uint8_t xsvf_tms_transitions[] = {
  TMS_T00, TMS_T01, TMS_T02, TMS_T03, TMS_T04, TMS_T05, TMS_T06, TMS_T07,
  TMS_T08, TMS_T09, TMS_T10, TMS_T11, TMS_T12, TMS_T13, TMS_T14, TMS_T15,
};
static const uint16_t xsvf_tms_map[] = {
  BS00, BS01, BS02, BS03, BS04, BS05, BS06, BS07,
  BS08, BS09, BS10, BS11, BS12, BS13, BS14, BS15
};
#endif



typedef struct jtag_port_t {
	uint8_t tms;
	uint8_t tdi;
	uint8_t tdo;
	uint8_t tck;
	uint8_t vref;
} jtag_port_t;

static void jtag_port_init(jtag_port_t* port) {
  pinMode(port->tms, OUTPUT);
  pinMode(port->tdi, OUTPUT);
  pinMode(port->tck, OUTPUT);
  pinMode(port->tdo, INPUT);
  pinMode(port->vref, INPUT);
}

static void jtag_port_pulse_clock(jtag_port_t* port) {
  digitalWrite(port->tck, 0);
  delayMicroseconds(1);
  digitalWrite(port->tck, 1);
}

static uint8_t jtag_port_pulse_clock_read_tdo(jtag_port_t* port) {
  uint8_t val;
  digitalWrite(port->tck, 0);
  delayMicroseconds(1);
  val = digitalRead(port->tdo);
  digitalWrite(port->tck, 1);
  return val;
}

static inline void jtag_port_set_tms(jtag_port_t* port, uint8_t val) {
  digitalWrite(port->tms, val);
}
static inline void jtag_port_set_tdi(jtag_port_t* port, uint8_t val) {
  digitalWrite(port->tdi, val);
}

static inline uint8_t jtag_port_get_veref(jtag_port_t* port) {
  return digitalRead(port->vref);
}


static uint8_t  xsvf_player_next_byte(void) {
  uint8_t retry = 16;
  uint8_t pos =  xsvf->rdpos % XSVF_BUF_SIZE;

  if (xsvf->wrpos == xsvf->rdpos) {
    size_t r = 0;
    while (r == 0) {
#if XSVF_DEBUG
      Serial.println("D<<< req read"); // request to receive BUF size bytes
#endif
      Serial.println(F("$062")); // request to receive BUF size bytes
      r = Serial.readBytes(xsvf_buf + pos, XSVF_BUF_SIZE - pos);
#if XSVF_DEBUG
      Serial.print("D<<< read "); // request to receive BUF size bytes
      Serial.println(r, DEC); // request to receive BUF size bytes
#endif
      if (r == 0) {
        retry --;
        if (retry == 0) {
          xsvf->error = 1;
          return 0;
        }
        delay(1);
      } else {
        xsvf->wrpos += r;
      }
    }
  }

  xsvf->rdpos++;
#if XSVF_DEBUG
   Serial.print(F("D BYTE "));
   Serial.print(xsvf_buf[pos], DEC);
   Serial.print(F(" 0x"));
   Serial.println(xsvf_buf[pos], HEX);
#endif
#if XSVF_CALC_CSUM
   xsvf->csum += xsvf_buf[pos];
#endif

  return xsvf_buf[pos];
}

static uint8_t xsvf_player_get_next_byte(void) {
  return xsvf_player_next_byte();
}
/*
static uint16_t xsvf_player_get_next_word(void) {
  uint16_t i = xsvf_player_next_byte();
  i <<= 8;
  i |= xsvf_player_next_byte();
  return i;
}
*/

static uint32_t xsvf_player_get_next_long(void) {
  uint32_t i = xsvf_player_next_byte();
  i <<= 8;
  i |= xsvf_player_next_byte();
  i <<= 8;
  i |= xsvf_player_next_byte();
  i <<= 8;
  i |= xsvf_player_next_byte();
  return i;
}

static uint32_t xsvf_player_get_next_bytes(uint8_t* data, uint32_t count) {
  while(count--) {
    *data++ = xsvf_player_next_byte();
  }
}

#ifdef XSVF_HEAP
static uint32_t xsvf_heap_pos(uint32_t* pos, uint16_t size) {
  uint32_t heap_pos = *pos;
  //allocate on 4 byte boundaries
  heap_pos = (heap_pos + 3) & 0xFFFFFFFC;
  *pos = heap_pos + size;
  return heap_pos;
}
#endif

static void xsvf_clear() {
  uint16_t i;
  uint8_t* d = (uint8_t*) xsvf;
  //clear the xsvf data in RAM
  i = sizeof(xsvf_t);
  while(i) {
    i--;
    d[i] = 0;
  }
}

static void xsvf_player_init(jtag_port_t* port) {
  jtag_port_init(port);

#ifdef XSVF_HEAP
  {
    // variables allocated on the heap
    uint32_t heap_pos = (uint32_t) XSVF_HEAP;

    xsvf = (xsvf_t*) xsvf_heap_pos(&heap_pos, sizeof(xsvf_t));
    xsvf_buf = (uint8_t*) xsvf_heap_pos(&heap_pos, XSVF_BUF_SIZE);

    xsvf_clear();

    xsvf->xsvf_tdo_mask = (uint8_t*) xsvf_heap_pos(&heap_pos, S_MAX_CHAIN_SIZE_BYTES);
    xsvf->xsvf_tdi = (uint8_t*) xsvf_heap_pos(&heap_pos, S_MAX_CHAIN_SIZE_BYTES);
    xsvf->xsvf_tdo = (uint8_t*) xsvf_heap_pos(&heap_pos, S_MAX_CHAIN_SIZE_BYTES);
    xsvf->xsvf_tdo_expected = (uint8_t*) xsvf_heap_pos(&heap_pos, S_MAX_CHAIN_SIZE_BYTES);
    xsvf->xsvf_address_mask = (uint8_t*) xsvf_heap_pos(&heap_pos, S_MAX_CHAIN_SIZE_BYTES);
    xsvf->xsvf_data_mask = (uint8_t*) xsvf_heap_pos(&heap_pos, S_MAX_CHAIN_SIZE_BYTES);
    xsvf_tms_transitions = (uint8_t*) xsvf_heap_pos(&heap_pos, 16);
    xsvf_tms_map = (uint16_t*) xsvf_heap_pos(&heap_pos, 32);

    if (heap_pos - ((uint32_t)XSVF_HEAP) > sizeof(XSVF_HEAP)) {
      Serial.print(F("Q-1,ERROR: Heap is small:"));
      Serial.println(heap_pos - ((uint32_t)XSVF_HEAP), DEC);
      return;
    }

    //set up TM transitions
    xsvf_tms_transitions[0] = TMS_T00;
    xsvf_tms_transitions[1] = TMS_T01;
    xsvf_tms_transitions[2] = TMS_T02;
    xsvf_tms_transitions[3] = TMS_T03;
    xsvf_tms_transitions[4] = TMS_T04;
    xsvf_tms_transitions[5] = TMS_T05;
    xsvf_tms_transitions[6] = TMS_T06;
    xsvf_tms_transitions[7] = TMS_T07;
    xsvf_tms_transitions[8] = TMS_T08;
    xsvf_tms_transitions[9] = TMS_T09;
    xsvf_tms_transitions[10] = TMS_T10;
    xsvf_tms_transitions[11] = TMS_T11;
    xsvf_tms_transitions[12] = TMS_T12;
    xsvf_tms_transitions[13] = TMS_T13;
    xsvf_tms_transitions[14] = TMS_T14;
    xsvf_tms_transitions[15] = TMS_T15;

    //set up bitstream map
    xsvf_tms_map[0] = BS00;
    xsvf_tms_map[1] = BS01;
    xsvf_tms_map[2] = BS02;
    xsvf_tms_map[3] = BS03;
    xsvf_tms_map[4] = BS04;
    xsvf_tms_map[5] = BS05;
    xsvf_tms_map[6] = BS06;
    xsvf_tms_map[7] = BS07;
    xsvf_tms_map[8] = BS08;
    xsvf_tms_map[9] = BS09;
    xsvf_tms_map[10] = BS10;
    xsvf_tms_map[11] = BS11;
    xsvf_tms_map[12] = BS12;
    xsvf_tms_map[13] = BS13;
    xsvf_tms_map[14] = BS14;
    xsvf_tms_map[15] = BS15;

  }
#else
  {
    xsvf_clear();

    xsvf->xsvf_tdo_mask = xsvf_tdo_mask;
    xsvf->xsvf_tdi = xsvf_tdi;
    xsvf->xsvf_tdo = xsvf_tdo;
    xsvf->xsvf_tdo_expected = xsvf_tdo_expected;
    xsvf->xsvf_address_mask = xsvf_address_mask;
    xsvf->xsvf_data_mask = xsvf_data_mask;
  }
#endif

  xsvf->repeat = 32;
  xsvf->endir_state = XSTATE_RUN_TEST_IDLE;
  xsvf->enddr_state = STATE_RUN_TEST_IDLE;
}


static void xsvf_jtagtap_state_ack(uint8_t tms) {
  tms <<= 2; // either 0 or 4
  xsvf->jtag_current_state = (xsvf_tms_transitions[xsvf->jtag_current_state] >> tms) & 0xf;
}

static void xsvf_jtagtap_shift_td(
  jtag_port_t* port,
	uint8_t *input_data,
	uint8_t *output_data,
	uint32_t data_bits,
	uint8_t must_end)
{
  uint32_t i, j;
	uint32_t bit_count = data_bits;
	uint32_t byte_count = (data_bits+ 7) >> 3;

	for (i = 0; i < byte_count; ++i) {
		uint8_t byte_out = input_data[byte_count - 1 - i];
		uint8_t tdo_byte = 0;
		for (j = 0; j < 8 && bit_count-- > 0; ++j) {
      uint8_t tdo;
			if (bit_count == 0 && must_end) {
				jtag_port_set_tms(port, 1);
				xsvf_jtagtap_state_ack(1);
			}
			jtag_port_set_tdi(port, byte_out & 1);
			byte_out >>= 1;
			tdo = jtag_port_pulse_clock_read_tdo(port);
			tdo_byte |= tdo << j;
		}
		output_data[byte_count - 1 - i] = tdo_byte;
	}
}

static void xsvf_jtagtap_state_step(jtag_port_t* port, uint8_t tms) {
  jtag_port_set_tms(port, tms);
  jtag_port_pulse_clock(port);
  xsvf_jtagtap_state_ack(tms);
}

static void xsvf_jtagtap_state_goto(jtag_port_t* port, uint8_t state) {
  if (xsvf->error) {
    return;
  }
  if (state == XSTATE_TEST_LOGIC_RESET) {
    uint8_t i;
		for (i = 0; i < 5; ++i) {
			xsvf_jtagtap_state_step(port, 1);
		}
	} else {
		while (xsvf->jtag_current_state != state) {
			xsvf_jtagtap_state_step(port, (xsvf_tms_map[xsvf->jtag_current_state] >> state) & 1);
		}
	}
}

static void xsvf_jtagtap_wait_time(jtag_port_t* port, uint32_t microseconds, uint8_t wait_clock) {
	uint32_t until;

  if (xsvf->error) {
    return;
  }

  until = micros() + microseconds;
  if (wait_clock) {
    while (microseconds--) {
      jtag_port_pulse_clock(port);
    }
  }
  while (micros() < until) {
    jtag_port_pulse_clock(port);
  }
}

static void xsvf_jtag_sir(jtag_port_t* port) {
  if (xsvf->error) {
    return;
  }
  xsvf_jtagtap_state_goto(port, XSTATE_SHIFT_IR);
  xsvf_jtagtap_shift_td(port, xsvf->xsvf_tdi, xsvf->xsvf_tdo, xsvf->sirsize_bits, 1);
  if (xsvf->runtest) {
    xsvf_jtagtap_state_goto(port, xsvf->endir_state);
  } else {
    xsvf_jtagtap_state_goto(port, XSTATE_RUN_TEST_IDLE);
    xsvf_jtagtap_wait_time(port, xsvf->runtest, 1);
  }
}


static uint8_t xsvf_jtag_is_tdo_as_expected(uint8_t use_mask)
{
  uint32_t i;
	for (i = 0; i < xsvf->sdrsize_bytes; ++i) {
		uint8_t expected = xsvf->xsvf_tdo_expected[i];
		uint8_t actual = xsvf->xsvf_tdo[i];
		if (use_mask) {
      uint8_t mask = xsvf->xsvf_tdo_mask[i];
			expected &= mask;
			actual &= mask;
		}
		if (expected != actual) {
#if XSVF_DEBUG
    Serial.println(F("D...NO MATCH!"));
#endif      
			return 0;
		}
	}

#if XSVF_DEBUG
    Serial.println(F("D...match!"));
#endif    
	return 1;
}


#define SDR_MUST_BEGIN (flags & 0b1000)
#define SDR_MUST_CHECK (flags & 0b0100)
#define SDR_USE_MASK   (flags & 0b0010)
#define SDR_MUST_END   (flags & 0b0001)

static uint8_t xsvf_jtag_sdr(jtag_port_t* port, uint8_t flags)
{
	int16_t attempts_left = xsvf->repeat;
	uint8_t matched = 0;
  uint8_t must_end = SDR_MUST_END;
  uint8_t must_check = SDR_MUST_CHECK;
  uint8_t use_mask = SDR_USE_MASK;

  if (xsvf->error) {
    return 0;
  }

	if (SDR_MUST_BEGIN) {
		xsvf_jtagtap_state_goto(port, XSTATE_SHIFT_DR);
	}
	while (!matched && attempts_left-- >= 0) {
    xsvf_jtagtap_shift_td(port, xsvf->xsvf_tdi, xsvf->xsvf_tdo, xsvf->sdrsize_bits, must_end);
		if (!must_check) {
			break;
		}
		matched = xsvf_jtag_is_tdo_as_expected(use_mask);
		if (!matched) {
			// XAP058, page 14
			xsvf_jtagtap_state_goto(port, XSTATE_PAUSE_DR);
			xsvf_jtagtap_state_goto(port, XSTATE_SHIFT_DR);
			xsvf_jtagtap_state_goto(port, XSTATE_RUN_TEST_IDLE);
			xsvf_jtagtap_wait_time(port, xsvf->runtest, 1);
			//
			xsvf_jtagtap_state_goto(port, XSTATE_SHIFT_DR);
#if XSVF_DEBUG
      if (attempts_left >= 0) {
        Serial.print(F("D...repeating: "));
        Serial.println(xsvf->repeat - attempts_left, DEC);
      }
#endif
		}
	}
	if (must_check && !matched) {
		xsvf->error = ERR_DR_CHECK_FAILED;
		Serial.println(F("D!DR check failed!"));
	}
	if (must_end && matched) {
		if (!xsvf->runtest) {
      xsvf_jtagtap_state_goto(port, xsvf->enddr_state);
		} else {
			xsvf_jtagtap_state_goto(port, XSTATE_RUN_TEST_IDLE);
			xsvf_jtagtap_wait_time(port, xsvf->runtest, 1);
		}
	}

	return !must_check || (must_check && matched);
}



/*
 * Reads the next instruction from the serial port. Also reads any
 * remaining instruction parameters into the instruction buffer.
 */
static uint8_t xsvf_player_handle_next_instruction(jtag_port_t* port) {
  uint8_t instruction = xsvf_player_next_byte();
  if (xsvf->error) {
    return ERR_IO; // failure
  }
  xsvf->instruction_counter++;

#if XSVF_DEBUG
   Serial.print(F("D INSTR "));
   Serial.print(xsvf->instruction_counter, DEC);
   Serial.print(F(" (0x"));
   Serial.print(instruction, HEX);
   Serial.print(F("): "));
#endif  

  //do not use switch as it uses RAM 
  // ---[COMPLETE ] --------------------------------------------
  if (instruction == XCOMPLETE) {
#if XSVF_DEBUG
   Serial.println(F("XCOMPLETE"));
#endif  
    xsvf->xcomplete = 1;
  } else

  // ---[TDO MASK] --------------------------------------------
  if (instruction == XTDOMASK) {
#if XSVF_DEBUG
   Serial.println(F("XTDOMASK"));
#endif  
    xsvf_player_get_next_bytes(xsvf->xsvf_tdo_mask, xsvf->sdrsize_bytes);
  } else

  // ---[SIR SIR2] --------------------------------------------
  if (instruction == XSIR || instruction == XSIR2) {
#if XSVF_DEBUG
   Serial.println(instruction == XSIR ? F("XSIR") : F("XSIR2"));
#endif  
    xsvf->sirsize_bits =  xsvf_player_get_next_byte();
    if (instruction == XSIR2) {
      xsvf->sirsize_bits <= 8;
      xsvf->sirsize_bits |= xsvf_player_get_next_byte();
    } 
    xsvf->sirsize_bytes = (xsvf->sirsize_bits + 7) >> 3;
    if (xsvf->sirsize_bytes > S_MAX_CHAIN_SIZE_BYTES) {
      return ERR_XSIR_SIZE;
    }
    xsvf_player_get_next_bytes(xsvf->xsvf_tdi, xsvf->sirsize_bytes);
    xsvf_jtag_sir(port);
  } else

  // ---[SDR ] --------------------------------------------
  if (instruction == XSDR || (instruction >= XSDRB && instruction <= XSDRE)) {
    uint8_t flags = 0b1111;
#if XSVF_DEBUG
   Serial.println(F("XSDRx"));
#endif  
    xsvf_player_get_next_bytes(xsvf->xsvf_tdi, xsvf->sdrsize_bytes);
    if (instruction != XSDR) {
      flags = (instruction == XSDRB) ? 0b1000 : (instruction == XSDRC) ? 0b0000 : 0b0001;
    }
    if (!xsvf_jtag_sdr(port, flags)) {
      xsvf->error = ERR_XSDR;
    }
  } else

  // ---[RUN TEST ] --------------------------------------------
  if (instruction == XRUNTEST) {
#if XSVF_DEBUG
   Serial.println(F("XRUNTEST"));
#endif  
    xsvf->runtest =  xsvf_player_get_next_long();
  } else
  // ---[REPEAT ] --------------------------------------------
  if (instruction == XREPEAT) {
#if XSVF_DEBUG
   Serial.println(F("XREPEAT"));
#endif  
    xsvf->repeat =  xsvf_player_get_next_byte();
  } else

  // ---[SDRSIZE ] --------------------------------------------
  if (instruction == XSDRSIZE) {
#if XSVF_DEBUG
   Serial.println(F("XSDRSIZE"));
#endif  
    xsvf->sdrsize_bits = xsvf_player_get_next_long();
    xsvf->sdrsize_bytes = (xsvf->sdrsize_bits + 7) >> 3;
    if (xsvf->sdrsize_bytes > S_MAX_CHAIN_SIZE_BYTES) {
      return ERR_XSDRSIZE;
    }
  } else

  // ---[SDRTDO ] --------------------------------------------
  if (instruction == XSDRTDO || (instruction >= XSDRTDOB && instruction <= XSDRTDOE)) {
    uint8_t flags = 0b1111;
#if XSVF_DEBUG
   Serial.println(F("XSDRTDOx"));
#endif  
    xsvf_player_get_next_bytes(xsvf->xsvf_tdi, xsvf->sdrsize_bytes);
    xsvf_player_get_next_bytes(xsvf->xsvf_tdo_expected, xsvf->sdrsize_bytes);
    if (instruction != XSDRTDO) {
      flags = (instruction == XSDRTDOB) ? 0b1100 : (instruction == XSDRTDOC) ? 0b0100 : 0b0101;
    }
    if (!xsvf_jtag_sdr(port, flags)) {
      xsvf->error = ERR_XSDR;
    }
  } else

  // ---[SET SDR MASKS ] --------------------------------------------
  if (instruction == XSETSDRMASKS) {
#if XSVF_DEBUG
   Serial.println(F("XSETSDRMASKS"));
#endif  
    xsvf_player_get_next_bytes(xsvf->xsvf_address_mask, xsvf->sdrsize_bytes);
    xsvf_player_get_next_bytes(xsvf->xsvf_data_mask, xsvf->sdrsize_bytes);
  } else

  // ---[SDR INC ] --------------------------------------------
  if (instruction == XSDRINC) {
#if XSVF_DEBUG
    Serial.println(F("XSDRINC"));
#endif  
    xsvf_player_get_next_bytes(xsvf->xsvf_tdi, xsvf->sdrsize_bytes);
    // TODO - check: return false?
  } else

  // ---[STATE ] --------------------------------------------
  if (instruction == XSTATE) {
#if XSVF_DEBUG
   Serial.println(F("XSTATE"));
#endif  
    xsvf->next_state =  xsvf_player_get_next_byte();
    xsvf_jtagtap_state_goto(port, xsvf->next_state);
  } else
  
  // ---[END IR ] --------------------------------------------
  if (instruction == XENDIR) {
    uint8_t s;
#if XSVF_DEBUG
   Serial.println(F("XENDIR"));
#endif  
    s = xsvf_player_get_next_byte();
    if (s == 0) {
      xsvf->endir_state = STATE_RUN_TEST_IDLE;
    } else
    if (s == 1) {
      xsvf->endir_state = STATE_PAUSE_IR;
    } else {
      return ERR_XENDIR;
    }
  } else

  // ---[END DR ] --------------------------------------------
  if (instruction == XENDDR) {
    uint8_t s;
#if XSVF_DEBUG
   Serial.println(F("XENDDR"));
#endif  

    s = xsvf_player_get_next_byte();
    if (s == 0) {
      xsvf->enddr_state = STATE_RUN_TEST_IDLE;
    } else
    if (s == 1) {
      xsvf->enddr_state = STATE_PAUSE_DR;
    } else {
      return ERR_XENDDR;
    }
  } else

  // ---[COMMENT ] --------------------------------------------
  if (instruction == XCOMMENT) {
    	uint8_t c;
#if XSVF_DEBUG
   Serial.println(F("XCOMMENT"));
#endif
      Serial.print(F("D"));//debug message preamble
      //read the comment bytes
    	do {
        c = xsvf_player_get_next_byte();
        // special feature: dump the TDO data
        if (c == '#') {
          uint8_t cnt = 0;
          uint8_t size = xsvf_player_get_next_byte() - '0';
          //dump the tdo buffer bytes
          while(cnt < size) {
            char t[4];
            uint8_t v = xsvf->xsvf_tdo[cnt];
            uint8_t x1 = v >> 4;
            v &= 0xF;
            // DEC to HEX conversion with leading zero
            t[0] = (char) (x1 < 10 ? '0' + x1 : 55 + x1 );
            t[1] = (char) (v < 10 ? '0' + v : 55 + v );
            t[2] = 0;
            Serial.print(t);
            cnt++;
          }
        }  else if (c) {
          Serial.print((char)c);
        }
      } while(c);
      Serial.println();
  } else

  // ---[WAIT ] --------------------------------------------
  if (instruction == XWAIT  || instruction == XWAITSTATE) {
    uint32_t clock_cnt = 0;
    uint8_t wait_clock = 1;
#if XSVF_DEBUG
   Serial.println(instruction == XWAIT ? F("XWAIT") : F("XWAITSTATE"));
#endif
  //TOOD - do we need these states to be global?
    xsvf->wait_start_state = xsvf_player_get_next_byte();
    xsvf->wait_end_state = xsvf_player_get_next_byte();
    if (instruction == XWAITSTATE) {
      clock_cnt = xsvf_player_get_next_long();
      wait_clock = clock_cnt > 0 ? 1 : 0;
    }
#if XSVF_DEBUG
   Serial.print(F("Dclock:"));
   Serial.println(clock_cnt, DEC);
#endif
   xsvf->wait_time_usecs = xsvf_player_get_next_long();
#if XSVF_DEBUG
   Serial.print(F("Dmicros:"));
   Serial.println( xsvf->wait_time_usecs, DEC);
#endif

    xsvf_jtagtap_state_goto(port, xsvf->wait_start_state);
    // happens only during XWAITSTATE
    while (clock_cnt) {
        jtag_port_pulse_clock(port);
        clock_cnt--;
    }
    xsvf_jtagtap_wait_time(port, xsvf->wait_time_usecs, wait_clock);
    xsvf_jtagtap_state_goto(port, xsvf->wait_end_state);

  } else
 // ---[TRST - test line reset] --------------------------------------------
  if (instruction == XTRST) {
#if XSVF_DEBUG
    Serial.println(F("XTRST"));
#endif
    //read test reset mode (0-on, 1-off, 2-Z, 3-Absent)
    xsvf_player_get_next_byte();
  } else
  // ---[UNKNOWN ] --------------------------------------------
  {
#if XSVF_DEBUG
   Serial.print(F("XUNKNOWN:"));
   Serial.println(instruction, DEC);
#endif
    //unimplemented instruction
    return ERR_INSTR_NOT_IMPLEMENTED;
  }

  if (xsvf->error) {
    return xsvf->error; // failure
  }
  return 0;
}


static void jtag_play_xsvf(jtag_port_t* port)
{
  uint32_t n = 0;
  uint8_t ret;

  xsvf_player_init(port);

  //check xref is high
  if (!jtag_port_get_veref(port)) {
    Serial.println(F("Q-255,JTAG not connected"));
    return;
  }

  Serial.println(F("RXSVF")); //announce ready to receive XSVF stream

  while(1) {
    n++;
    ret = xsvf_player_handle_next_instruction(port); 
    if (ret) {
      Serial.print(F("Q-"));
      Serial.print(ret, DEC );
      Serial.println(F(",Fail"));
      break;
    } else {
      if (xsvf->xcomplete) {
        Serial.println(F("!Success"));
        break;
      }
    }
  }
  Serial.print(F("!Processed instr:"));
  Serial.println(xsvf->instruction_counter, DEC);

#if XSVF_CALC_CSUM
  Serial.print(F("!sum: 0x"));
  // print leading zeros in the check sum hex value
  {
    uint32_t i = 0xF0000000;
    while((!(xsvf->csum & i)) && i) {
      Serial.print(F("0"));
      i >>= 4;
    }
  }
  Serial.print(xsvf->csum, HEX);
  Serial.print(F("/"));
  Serial.println(xsvf->rdpos, DEC);
#endif /* XSVF_CALC_CSUM */

  if (xsvf->xcomplete) {
      Serial.println(F("Q-0,OK"));
  }
  // put the jtag port pins into High-Z (vref already is input)
  pinMode(port->tms, INPUT);
  pinMode(port->tdi, INPUT);
  pinMode(port->tck, INPUT);
  pinMode(port->tdo, INPUT);
}


#endif /*_JTAG_XSVF_PLAYER_H_*/
