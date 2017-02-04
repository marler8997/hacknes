#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "ppu.h"
#include "graphics.h"

#define CYCLES_PER_SCANLINE 341
#define SCANLINES_PER_FRAME 262

PpuPalette ppuPalette[PPU_PALETTE_SIZE] = {
  84 , 84,  84,      0,  30, 116,      8,  16, 144,     48,   0, 136,
  68 ,  0, 100,     92,   0,  48,     84,   4,   0,     60,  24,   0,
  0  , 50,  60,      0,   0,   0,    152, 150, 152,      8,  76, 196,
  48 , 50, 236,     92,  30, 228,    136,  20, 176,    160,  20, 100,
  152, 34,  32,    120,  60,   0,     84,  90,   0,     40, 114,   0,
  8  ,124,   0,      0, 118,  40,      0, 102, 120,      0,   0,   0,
  236,238, 236,     76, 154, 236,    120, 124, 236,    176,  98, 236,
  228, 84, 236,    236,  88, 180,    236, 106, 100,    212, 136,  32,
  160,170,   0,    116, 196,   0,     76, 208,  32,     56, 204, 108,
  56 ,180, 204,     60,  60,  60,    236, 238, 236,    168, 204, 236,
  188,188, 236,    212, 178, 236,    236, 174, 236,    236, 174, 212,
  236,180, 176,    228, 196, 144,    204, 210, 120,    180, 222, 120,
  168,226, 144,    152, 226, 180,    160, 214, 228,    160, 162, 160
};



// Control Register 1 Values
ubyte controlRegister1 = 0;
struct DecodedControlRegister
{
  ushort autoIncrement;
  ushort spritePatternTableAddr;
  ushort backgroundPatternTableAddr;
  bool sprite8X16Mode;
  bool vblankNmiEnabled;
  DecodedControlRegister() :
    autoIncrement              ((controlRegister1 & PPU_CONTROL_REGISTER_1_AUTO_INCREMENT_32) ? 32 : 1),
    spritePatternTableAddr     ((controlRegister1 & PPU_CONTROL_REGISTER_1_SPRITE_TABLE_LOC) ? 0x1000 : 0),
    backgroundPatternTableAddr ((controlRegister1 & PPU_CONTROL_REGISTER_1_BACKGROUND_TABLE_LOC) ? 0x1000 : 0),
    sprite8X16Mode             ((controlRegister1 & PPU_CONTROL_REGISTER_1_8X16_SPRITE_MODE) ? true : false),
    vblankNmiEnabled           ((controlRegister1 & PPU_CONTROL_REGISTER_1_ENABLE_VBLANK_NMI) ? true : false)
  {
  }
};

// Status Register
ubyte statusRegister = 0;

// 2 pattern tables, each $1000 bytes long
// each sprite in the pattern table is 16 bytes long.
ubyte patternTables[0x2000];

// Each nametable is 1K (1024 bytes or 0x400 bytes)
// The PPU only has memory for 2 nametables, so only 2K.
// However, it has address space for 4 nametables.
// The ppu can either mirror the 2 nametables, or use
// VRAM on the cartridge for the other 2 namtables.
// This mirroring is determined by the mirrorType, and
// implemented by setting the nameTableMemoryMap.
ubyte nameTables[0x800]; // 2KB for nametable ram
ubyte* nameTableMemoryMap[4]; // Pointer to each nametable.


// First 16 bytes  (Image palette)
// Second 16 bytes (Sprite palette)
// The palette are indexes into the system palette.  The
// system paletts is 64 bytes long, so each index in this
// palette is masked to only be 6 bits.
//
// Note: palette entry at $3F00 is the background color and used for transparency.
// Note: the background color is "mirrored" 4 times.
// $3F00 == $3F04 == $3F08 == $3F0C == $3F10 == $3F14 == $3F18 == $3F1C
// So each pallete really only has 13 colors.
// So since there are 2 palettes, the total number of colors on screen at any time is
// only 25.
//
ubyte imageAndSpritePalettes[32];

static ubyte* pixelBuffer;
int PpuInit(MirrorType mirrorType)
{
  // Setup nameTableMaps based on mirrorType
  switch(mirrorType) {
  case MIRROR_TYPE_HORIZONTAL:
    nameTableMemoryMap[0] = &nameTables[0x000];
    nameTableMemoryMap[1] = &nameTables[0x000];
    nameTableMemoryMap[2] = &nameTables[0x400];
    nameTableMemoryMap[3] = &nameTables[0x400];
    break;
  case MIRROR_TYPE_VERTICAL:
    nameTableMemoryMap[0] = &nameTables[0x000];
    nameTableMemoryMap[1] = &nameTables[0x400];
    nameTableMemoryMap[2] = &nameTables[0x000];
    nameTableMemoryMap[3] = &nameTables[0x400];
    break;
  case MIRROR_TYPE_NONE_USE_VRAM:
    printf("PpuInit: Error: mirror type non, use vram is not implemented\n");
    return 1; // fail
  default:
    printf("PpuInit: Error: unknown mirror type $%02X\n", mirrorType);
    return 1; // fail
  }

  if(graphicsEnabled) {
    pixelBuffer = getPixelBuffer();
  } else {
    pixelBuffer = (ubyte*)malloc(SCREEN_PIXEL_WIDTH*SCREEN_PIXEL_HEIGHT);
  }
  return (pixelBuffer == NULL) ? 1 : 0;
}

#define LOG_IO(fmt,...)
//#define LOG_IO(fmt,...) printf(fmt"\n", ##__VA_ARGS__)

ubyte PpuIOReadForLog(ubyte addr)
{
  if(addr == 0x02) {
    return statusRegister;
  } else if(addr == 0x07) {
    return 0; // NOT: NOT IMPLEMENTED!
  }
  return 0;
}

// Assumption: 0 <= addr <= 7
ubyte PpuIORead(ubyte addr)
{
  if(addr == 0x02) {
    LOG_IO("[DEBUG] PpuIORead $02 (StatusRegister) $%02X", statusRegister);
    return statusRegister;
  } else if(addr == 0x07) {
    LOG_IO("[DEBUG] PpuIORead $07 NOT IMPLEMENTED");
    return 0;
  }
  printf("WARNING: PpuIORead $%02X is invalid\n", addr);
  return 0;
}
// Assumption: 0 <= addr <= 7
void PpuIOWrite(ubyte addr, ubyte value)
{
  LOG_IO("[DEBUG] PpuIOWrite $%02X value $%02X (%d) (%u)",
	 addr, value, value, value);
  switch(addr) {
  case 0:
    controlRegister1 = value;
    {
      DecodedControlRegister decoded;
      printf("ppu control_1: inc %u, sprite_loc $%04X, bg_loc $%04X, 8x16 %u, vblank_nmi %u\n",
	     decoded.autoIncrement, decoded.spritePatternTableAddr, decoded.backgroundPatternTableAddr,
	     decoded.sprite8X16Mode, decoded.vblankNmiEnabled);
    }
    break;
  case 1:
    printf("PpuIOWrite 1 not implemented\n");
    break;
  case 2:
    printf("PpuIOWrite 2 not implemented\n");
    break;
  case 3:
    printf("PpuIOWrite 3 not implemented\n");
    break;
  case 4:
    printf("PpuIOWrite 4 not implemented\n");
    break;
  case 5:
    printf("PpuIOWrite 5 not implemented\n");
    break;
  case 6:
    printf("PpuIOWrite 6 not implemented\n");
    break;
  case 7:
    printf("PpuIOWrite 7 not implemented\n");
    break;
  }
}

// TODO: Using a generic memory mapped function like this
//       may not be the most efficient way to do most things
//       in the PPU. Then again, this function is actually pretty
//       small...not sure.
ubyte PpuReadByte(ushort addr)
{
  addr &= 0x3FFF; // Normalize $4000 - $FFFF to $0000 - $3FFF

  if(addr < 0x2000) { // Pattern tables
    return patternTables[addr];
  }
  if(addr < 0x3F00) { // Name tables
    // The name table index will be in these 2
    // bits: ---- XX-- ---- ----
    return *nameTableMemoryMap[(addr >> 10) & 0x3];
  }
  
  // image and sprite color palettes
  // The first $20 bytes are mirrored up to $3FFF
  // Addresses are normalized to the first $20 by masking with $1F.
  return imageAndSpritePalettes[addr & 0x1F];
}


static size_t ppuStepCount = 0;

PpuState ppuState;

void renderPixel()
{
  
  //printf("renderPixel (%u x %u)\n", currentScanlineCycle, currentScanline - 21);
}

void ppuStep()
{
  //printf("------------------------------\n");
  //printf("ppuStep %u, scanline %u, scanline_cycle %u\n",
  //ppuStepCount, ppuState.scanline, ppuState.scanlineCycle);
  ppuStepCount++;

  if(ppuState.scanline >= 21 && ppuState.scanline <= 260) {
    if(ppuState.scanlineCycle < SCREEN_PIXEL_WIDTH) {
      renderPixel();
    }
  }

  ppuState.scanlineCycle++;
  if(ppuState.scanlineCycle == CYCLES_PER_SCANLINE) {
    ppuState.scanline++;
    ppuState.scanlineCycle = 0;
    if(ppuState.scanline == 20) {
      // The VBL flag is cleared at 6820 PPU clocks, or exactly 20 scanlines
      statusRegister &= ~PPU_STATUS_REGISTER_VBLANK;
      // TODO: setStatusSpriteOverflow to FALSE
      // TODO: setStatusSpriteCollisionHit to FALSE
    } else if(ppuState.scanline == SCANLINES_PER_FRAME) {
      //printf("ppu: FRAME DONE!\n");
      statusRegister |= PPU_STATUS_REGISTER_VBLANK;
      //printf("ppu: set vblank!\n");
      ppuState.scanline = 0;
      if(controlRegister1 & PPU_CONTROL_REGISTER_1_ENABLE_VBLANK_NMI) {
	// TODO: generate NMI
	//printf("ppu: generate NMI!\n");
      }
    }
  }
}
