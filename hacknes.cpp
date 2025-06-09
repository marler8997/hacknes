#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "platform.h"
#include "cartridge.h"
#include "graphics.h"
#include "audio.h"
#include "cpu.h"
#include "ppu.h"

extern "C" {

bool graphicsEnabled = true;

int hacknes(const char* romFile, bool audioEnabled, int startAddress)
{
  if(!cartridge.load(romFile)) {
    return 1;
  }

  if(startAddress >= 0) {
    uint16_t startAddressU16 = startAddress;
    if (startAddressU16 != startAddress) abort();

    printf("Setting start address to $%04X\n", startAddress);
    size_t secondBankOffset = (cartridge.prgRomSize >= 2*_16K) ?
      _16K : 0;
    cartridge.prgRom[secondBankOffset+0x3FFC] = (ubyte)startAddressU16;
    cartridge.prgRom[secondBankOffset+0x3FFD] = (ubyte)(startAddressU16 >> 8);
  }

  if(graphicsEnabled) {
    if(!setupGraphics()) {
      return 1;
    }
  }
  if(audioEnabled) {
    if(!setupAudio()) {
      return 1;
    }
  }

  // Setup graphics
  {
    int result = PpuInit(cartridge.mirrorType);
    if(result) {
      if(graphicsEnabled) cleanupGraphics();
      return 1;
    }
  }

  return runCpu();
}

}
