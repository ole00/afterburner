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

// compacting statistics - disabled by default
#define COMPACT_STAT 0

#define SPFUSES 128
unsigned char fuseType[SPFUSES]; //sparse fuses index

uint16_t sparseFusemapStat = 0; //bit 15: use sparse fusemaps, bits 0-11 : sparse fusemap size in bytes
uint8_t  sparseCompactCounter = 0;
uint16_t sparseCacheBitPos = 0;
uint16_t sparseCacheOffset = 0;
uint16_t  sparseCacheHit = 0;
uint8_t  sparseCacheIndex = 0;

#if COMPACT_STAT
uint8_t sparseCompactRun = 0;
uint8_t sparseCompactAct = 0;
#endif


// reverse search of the fuse group index based on the byte position in the sparse array
// returns the group index
static uint16_t getFuseGroupIndex(uint16_t fuseOffsetBytePos) {
  uint16_t groupPos = 0;
  uint16_t i = 0;
  uint16_t fuseOffset = 0;

  //calculate fusemap offset
  while (1) {
    uint8_t rec = fuseType[i];
    // speed optimised special case: all 8 bits are 0 (4 * 32 bits)
    if (rec == 0) {
        groupPos += 4; //4 groups in the byte
    } else {
      uint8_t j = 0;
      //4 types per byte
      while (j < 4) {
        if ((rec & 0b11) == 1) { // type 0 & 3 - no byte stored in fusemap
          if (fuseOffset == fuseOffsetBytePos) {
              return groupPos;
          }
          fuseOffset += 4; // 32 bits in the group, fuse byte offset advances by 4 bytes
        }
        groupPos++; //check next group
        rec >>= 2;
        j++;
      }
    }
    i++; //next byte from the fuseTypes
  }
}


// get position of the fuse bit in the sparse array
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


static void sparseCompactFuseMap(void) {
  uint16_t i, j;
  uint16_t total = (sparseFusemapStat & 0x7FF) >> 2; // max is 2048 bytes, and convert to the group index
  uint32_t* fuses = (uint32_t*) fusemap;

  if (total < 2) {
    return;
  }

#if COMPACT_STAT
  sparseCompactRun++; //statistics
#endif

  i = total - 1;
  while(i) {
    // remove 4 fusemap bytes at a position when the bits are all 1's
    if (fuses[i] == 0xFFFFFFFF) {
      if (i < total - 1) {
        uint16_t fuseGroup = getFuseGroupIndex(i << 2); //ensure the int32 index is converted to int8/byte index
#if COMPACT_STAT
        sparseCompactAct++; //statistics
#endif
        /// shift the fuses by 4 bytes to the left
        for (j = i; j < total - 1 ; j++) {
          fuses[j] = fuses[j + 1];
        }
        total--;
        fuseType[fuseGroup >> 2] |= (3 << ((fuseGroup & 0b11) << 1)); //set type 3 at the fuse group record
        sparseFusemapStat -= 4; // fuse map total size reduced by 4 bytes
      }
    }
    i--;
  }
  //destory cache
  sparseCacheBitPos = 0;
  sparseCacheOffset = 0;
  sparseCacheIndex = 0;
}

static inline uint16_t sparseSetFuseBit(uint16_t bitPos) {
    uint8_t type;
    uint16_t pos;

    //try to reduce the size of the fuse map by finding and removing blocks with all 1's
    sparseCompactCounter ++;
    if (sparseCompactCounter == 255) {
      sparseCompactFuseMap();
    }

    pos = getFusePositionAndType(bitPos);
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
    Serial.print(F("c_hit="));
    Serial.println(sparseCacheHit, DEC);
#if COMPACT_STAT
    Serial.print(F("compact run="));
    Serial.print(sparseCompactRun, DEC);
    Serial.print(F(" cnt="));
    Serial.println(sparseCompactAct, DEC);
#endif
}

static void sparseInit(char clearArray) {
  if (clearArray) {
    uint8_t i;
    for (i = 0; i < SPFUSES; i++) {
      fuseType[i] = 0;
    }

  }
  sparseFusemapStat = (1 << 15);
  sparseCompactCounter = 0;
  sparseCacheBitPos = 0;
  sparseCacheOffset = 0;
  sparseCacheIndex = 0;
#if COMPACT_STAT
  sparseCompactRun = 0;
  sparseCompactAct = 0;
#endif
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
