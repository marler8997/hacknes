#pragma once

struct Cartridge
{
  ubyte* prgRom; // PRG Read-Only memory
  size_t prgRomSize;

  ubyte* chrRom; // CHR Read-Only memory
  size_t chrRomSize;

  ~Cartridge()
  {
    // TODO: maybe it would be better to put prgRom and chrRom in the
    // same malloc'd buffer.
    if(prgRom) {
      //printf("[DEBUG] Cartridge: freeing prg memory...\n");
      free(prgRom);
    }
    if(chrRom) {
      free(chrRom);
    }
  }

  bool load(const char* filename);
};
