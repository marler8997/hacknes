#pragma once

#define SCREEN_PIXEL_WIDTH  256
#define SCREEN_PIXEL_HEIGHT 240

// Register 0 (Control Register 1) (Write Only)
// Register 1 (Control Register 2) (Write Only)
// Register 2 (Status Register) (Read Only)

#define PPU_CONTROL_REGISTER_1_NAME_TABLE_MASK      0x03
#define PPU_CONTROL_REGISTER_1_AUTO_INCREMENT_32    0x04
#define PPU_CONTROL_REGISTER_1_SPRITE_TABLE_LOC     0x08
#define PPU_CONTROL_REGISTER_1_BACKGROUND_TABLE_LOC 0x10
#define PPU_CONTROL_REGISTER_1_8X16_SPRITE_MODE     0x20
#define PPU_CONTROL_REGISTER_1_ENABLE_VBLANK_NMI    0x80

#define PPU_CONTROL_REGISTER_2_MONOCHROME_MODE      0x01
#define PPU_CONTROL_REGISTER_2_SHOW_FULL_BACKGROUND 0x02
#define PPU_CONTROL_REGISTER_2_SHOW_FULL_SPRITES    0x04
#define PPU_CONTROL_REGISTER_2_ENABLE_BACKGROUND    0x08
#define PPU_CONTROL_REGISTER_2_ENABLE_SPRITES       0x10
#define PPU_CONTROL_REGISTER_2_BG_COLOR_MASK        0xE0

#define PPU_STATUS_REGISTER_ACCEPTING_WRITES 0x10
#define PPU_STATUS_REGISTER_VBLANK           0x80

// NOTE: Reading the Status registers clears the VBLANK bit,
//       and also registers 5 and 6.

#define PPU_PALETTE_SIZE 64
struct PpuPalette
{
  ubyte red;
  ubyte green;
  ubyte blue;
  /*
  PpuPalette(ubyte red, ubyte green, ubyte blue) : red(red), green(green), blue(blue)
  {
  }
  */
};
extern PpuPalette ppuPalette[PPU_PALETTE_SIZE];

// Returns: 0 on success
int PpuInit(MirrorType mirrorType);

ubyte PpuIOReadForLog(ubyte addr);
// Assumption: 0 <= addr <= 7
ubyte PpuIORead(ubyte addr);
// Assumption: 0 <= addr <= 7
void PpuIOWrite(ubyte addr, ubyte value);

void ppuStep();

struct PpuState
{
  ushort scanline;
  ushort scanlineCycle;
  PpuState()
  {
  }
};
extern PpuState ppuState;
