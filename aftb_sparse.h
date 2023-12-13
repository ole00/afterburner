/*
 * Sparse fusemap functions for Afterburner GAL project.
 * 
 *  The idea of sparse fuse map is to store non-zero fuse bits only.
 *  The reason is to fit big fusemaps into a size-limited SRAM.
 *  The fusemap is divided into groups of 16 bits and each group
 *  has a 'type' bit stored in fuseTypes array. Group type 0 has
 *  all 16 fuesmap bits 0 and is not stored in fusemap array.
 *  Group type 1 has at least 1 fusemap bit set and is stored in 
 *  fusemap array. Position of group type '1' in the fusemap array
 *  may vary based on which fuses are written.
 *
 *  Sparse fusemap supports:
 *  - random reads and writes
 *  - a simple cache to speed up index look-ups.
 */

#ifdef USE_SPARSE_FUSEMAP

#define SPFUSES 128
unsigned char fuseType[SPFUSES]; //sparse fuses index

uint16_t sparseFusemapStat = 0; //bit 15: use sparse fusemaps, bits 0-11 : sparse fusemap size in bytes
uint16_t sparseCacheBitPos = 0;
uint16_t sparseCacheOffset = 0;
uint16_t  sparseCacheHit = 0;
uint8_t  sparseCacheIndex = 0;

static uint16_t getFusePositionAndType(uint16_t bitPos) {
  uint16_t counter = 0;
  uint8_t i = 0;
  uint8_t type;
  uint16_t fuseOffset = (bitPos & 0b11000)  >> 3; //set odd / even byte of the fuse offset

  if (bitPos <= sparseCacheBitPos) {
    sparseCacheBitPos = 0;
    sparseCacheOffset = 0;
    sparseCacheIndex = 0;
  } else {
    counter = sparseCacheBitPos;
    fuseOffset += sparseCacheOffset & 0xFFC;
    i = sparseCacheIndex;
    sparseCacheHit++;
  }

  //calculate fusemap offset
  while (1) {
    uint8_t rec = fuseType[i];
    // speed optimised special case: all 8 bits are 0 (4 * 32 bits)
    if (rec == 0) {
        counter += 128;
        if (counter > bitPos) {
          return (fuseOffset << 2); // type is 0
        }
        sparseCacheBitPos = counter;
        sparseCacheOffset = fuseOffset;
        sparseCacheIndex = i + 1;
    } else {
      uint8_t j = 0;
      //4 fuse types per byte
      while (j < 4) {
        counter += 32;
        type = rec & 0b11;
        if (counter > bitPos) {
          return (fuseOffset << 2) | type;
        }
        if (type == 1) { // type 0 & 3 - no byte stored in fusemap
          fuseOffset += 4;
        }
        rec >>= 2;
        j++;
      }
    }
    i++; //next byte from the fuseTypes
  }
}

static void insertFuseGroup(uint16_t dataPos, uint16_t bitPos) {
  int16_t i = bitPos >> 5; //group index
  uint16_t totalFuseBytes = sparseFusemapStat & 0x7FF; // max is 2048 bytes
  fuseType[i >> 2] |= (1 << ((i & 0b11) << 1)); // set type 1 at the fuse group record

  //shift all data in the fuse map  starting at data pos by 4 bytes (32 bits)
  if (dataPos < totalFuseBytes) {
    for (i =  totalFuseBytes - 1; i >= dataPos; i--) {
      fusemap[i + 4] = fusemap[i];
    }
  }
  sparseFusemapStat = totalFuseBytes + 4; // we can ignore the sparse bit
  //clean the emptied fusemap data
  fusemap[dataPos++] = 0;
  fusemap[dataPos++] = 0;
  fusemap[dataPos++] = 0;
  fusemap[dataPos] = 0;
}

static inline uint16_t sparseSetFuseBit(uint16_t bitPos) {
    uint8_t type;
    uint16_t pos = getFusePositionAndType(bitPos);
    type = pos & 0b11;
    pos >>= 2; //trim the type to get the byte position in fuse map
    if (type == 0) { //we need to write the bit into a group that has all bits 0 so far
      insertFuseGroup(pos & 0x7FC, bitPos);
    }
    return pos;
}

static inline uint16_t sparseGetFuseBit(uint16_t bitPos) {
    uint8_t type;
    uint16_t pos = getFusePositionAndType(bitPos);

    type = pos & 0b11;
    if (!type) { //type is 0 - the block contains all zero bits
      return 0xFF00;
    }
    if (type == 3) {  // type is 3 - the block contains all 1 bits
      return 0xFF01;
    }
    pos >>= 2; //trim the type to get byte position in fuse map
    return pos;
}

static void sparsePrintStat() {
    Serial.print(F("sp bytes="));
    Serial.println(sparseFusemapStat & 0x7FF, DEC);
    Serial.print("chit=");
    Serial.println(sparseCacheHit, DEC);
}

static void sparseInit(char clearArray) {
  if (clearArray) {
    uint8_t i;
    for (i = 0; i < SPFUSES; i++) {
      fuseType[i] = 0;
    }

  }
  sparseFusemapStat = (1 << 15);
  sparseCacheBitPos = 0;
  sparseCacheOffset = 0;
  sparseCacheIndex = 0;
}
static inline void sparseDisable(void) {
    sparseFusemapStat = 0;
}
#else /* ! USE_SPARSE_FUSEMAP */

#define sparseDisable()
#define sparseInit(X)
#define sparseGetFuseBit(X) 0
#define sparseSetFuseBit(X) 0
#define sparsePrintStat()
#define sparseFusemapStat 0
#endif
