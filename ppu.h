#pragma once

// Register 0 (Control Register 1) (Write Only)
// Register 1 (Control Register 2) (Write Only)
// Register 2 (Status Register) (Read Only)

#define PPU_CONTROL_REGISTER_1_NAME_TABLE_MASK      0x03
#define PPU_CONTROL_REGISTER_1_AUTO_INCREMENT_32    0x04
#define PPU_CONTROL_REGISTER_1_SPRITE_TABLE_LOC     0x08
#define PPU_CONTROL_REGISTER_1_BACKGROUND_TABLE_LOC 0x10
#define PPU_CONTROL_REGISTER_1_8X16_SPRITE_MODE     0x20
#define PPU_CONTROL_REGISTER_1_ENABLE_VBLANK_NMI    0x80

#define PPU_CONTROL_REGISTER_2_HIDE_BACKGROUND   0x08
#define PPU_CONTROL_REGISTER_2_HIDE_SPRITES      0x10

#define PPU_STATUS_REGISTER_ACCEPTING_WRITES 0x10
#define PPU_STATUS_REGISTER_VBLANK           0x80

// NOTE: Reading the Status registers clears the VBLANK bit,
//       and also registers 5 and 6.
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
