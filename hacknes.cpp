#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "platform.h"
#include "cartridge.h"
#include "graphics.h"
#include "audio.h"
#include "cpu.h"
#include "ppu.h"

bool audioEnabled = true;
bool graphicsEnabled = true;
static int overrideStartAddress = -1;

void PrintUsage()
{
  printf("Usage: hacknes [options] <rom-file>\n");
  printf("  --no-audio               Disable audio\n");
  printf("  --no-graphics            Disable graphics\n");
  printf("  --start-address <hex>    Address to start execution\n");
}
int ParseCommandLine(int argc, const char* argv[])
{
  int newArgc = 1;
  for(int i = 1; i < argc; i++) {
    const char* currentArg = argv[i];
    if(currentArg[0] != '-') {
      argv[newArgc++] = currentArg;
    } else {
      if(strcmp(currentArg, "--no-audio") == 0) {
        audioEnabled = false;
      } else if(strcmp(currentArg, "--no-graphics") == 0) {
        graphicsEnabled = false;
      } else if(strcmp(currentArg, "--start-address") == 0) {
        i++;
        if(i >= argc) {
          printf("Error: option --start-address requres an argument\n");
          return -1;
        }
        currentArg = argv[i];
        sscanf(currentArg, "%x", &overrideStartAddress);
        printf("Start address will be $%04X\n", overrideStartAddress);
      } else {
        printf("Error: unknown option \"%s\"\n", currentArg);
        return -1;
      }
    }
  }
  return newArgc;
}

int main(int argc, const char* argv[])
{
  argc = ParseCommandLine(argc, argv);
  if(argc < 0) {
    return 1;
  }

  if(argc <= 1) {
    PrintUsage();
    return 1;
  }
  if(argc > 2) {
    printf("Error: too many command line args\n");
    return 1;
  }

  const char* romFile = argv[1];
  if(!cartridge.load(romFile)) {
    return 1;
  }

  if(overrideStartAddress >= 0) {
    printf("Setting start address to $%04X\n", overrideStartAddress);
    size_t secondBankOffset = (cartridge.prgRomSize >= 2*_16K) ?
      _16K : 0;
    cartridge.prgRom[secondBankOffset+0x3FFC] = (ubyte)overrideStartAddress;
    cartridge.prgRom[secondBankOffset+0x3FFD] = (ubyte)(overrideStartAddress >> 8);
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
