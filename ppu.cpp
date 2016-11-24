#include <stdio.h>
#include "common.h"
#include "ppu.h"

#define PIXEL_SCREEN_WIDTH  256
#define CYCLES_PER_SCANLINE 341
#define SCANLINES_PER_FRAME 262


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

//ubyte vram[0x4000]; // 16 KB of ram

// 2 pattern tables, each $1000 bytes long
// each sprite in the pattern table is 16 bytes long.
ubyte patternTables[0x2000];


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


#define LOG_IO(fmt,...)
//#define LOG_IO(fmt,...) printf(fmt"\n", ##__VA_ARGS__)

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

ubyte PpuReadByte(ushort addr)
{
  addr &= 0x3FFF; // Normalize $4000 - $FFFF to $0000 - $3FFF

  if(addr < 0x3F00) {
    printf("TODO: implement PpuReadByte address < 0x3F00\n");
    return 0;
  }

  if(addr < 0x3FFF) { // image and sprite color palettes
    // The first $20 bytes are mirrored up to $3FFF
    // Addresses are normalized to the first $20 by masking with $1F.
    return imageAndSpritePalettes[addr & 0x1F];
  }
  
  printf("TODO: implement PpuReadByte address > 0x3F1F\n");
  return 0;
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
    if(ppuState.scanlineCycle < PIXEL_SCREEN_WIDTH) {
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
      printf("ppu: FRAME DONE!\n");
      statusRegister |= PPU_STATUS_REGISTER_VBLANK;
      printf("ppu: set vblank!\n");
      ppuState.scanline = 0;
      if(controlRegister1 & PPU_CONTROL_REGISTER_1_ENABLE_VBLANK_NMI) {
	// TODO: generate NMI
	printf("ppu: generate NMI!\n");
      }
    }
  }
}
