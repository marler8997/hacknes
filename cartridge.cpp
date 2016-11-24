#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "common.h"
#include "cartridge.h"

#define FLAGS6_TRAINER_MASK 0x04

struct ScopedFile
{
  FILE *ptr;
  ScopedFile(FILE *ptr) : ptr(ptr)
  {
  }
  ~ScopedFile()
  {
    if(ptr) {
      fclose(ptr);
    }
  }
};

void printFlags6(ubyte flags)
{
  printf("flags6    0x%02x\n", flags);
  printf(" 1111_0000: %u Lower nybble of mapper number\n", flags & 0xF0);
  printf(" 0000_1000: %u Graphics type (bit 1)\n", flags & 0x08);
  printf(" 0000_0100: %u 512-byte trainer at $7000-$71FF (stored before PRG data)\n", flags & FLAGS6_TRAINER_MASK);
  printf(" 0000_0010: %u Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other persistent memory\n", flags & 0x20);
  printf(" 0000_0001: %u Grapics type (bit 2)\n", flags & 0x01);
  if(flags & 0x08) {
    printf(" Graphics: four-screen VRAM (flags ----1---)\n");
  } else {
    if(flags & 0x01) {
      printf(" Graphics: vertical arrangement/horizontal mirroring (CIRAM A10 = PPU A11) (flags ----0--1)\n");
    } else {
      printf(" Graphics: horizontal arrangement/vertical mirroring (CIRAM A10 = PPU A10) (flags ----0--0)\n");
    }
  }
}
void printFlags7(ubyte flags)
{
  printf("flags7    0x%02x\n", flags);
  printf(" 1111_0000: %u Upper nybble of mapper number\n", flags & 0xF0);

  // TODO: print the rest
  //
  // 76543210
  // ||||||||
  // |||||||+- VS Unisystem
  // ||||||+-- PlayChoice-10 (8KB of Hint Screen data stored after CHR data)
  // ||||++--- If equal to 2, flags 8-15 are in NES 2.0 format
  // ++++----- Upper nybble of mapper number
}
void printFlags9(ubyte flags)
{
  printf("flags9    0x%02x\n", flags);
  printf(" 0000_0001: %u TV system (0:NTSC; 1: PAL)\n", flags&0x01);
}

// Returns: false on failure, logs error
bool Cartridge::load(const char* filename)
{
  ScopedFile file(fopen(filename, "rb"));
  if(!file.ptr) {
    printf("Cartridge load: Error: failed to open \"%s\" (e=%d)\n", filename, errno);
    return false; // fail
  }

  ubyte header[16];
  {
    size_t bytesRead = fread(header, 1, 16, file.ptr);
    if(bytesRead != 16) {
      printf("Cartridge load: Error: fread of 16-byte header returned %u (e=%d)\n",
	     bytesRead, errno);
      return false; // fail
    }
  }

  if(header[0] != 'N' ||
     header[1] != 'E' ||
     header[2] != 'S' ||
     header[3] != 0x1A) {
    printf("Error: invalid rom file (mismatched HEADER)\n");
    return false;
  }
  printf("Valid ROM header!\n");

  #define PRG_ROM_16K_SIZE_INDEX 4
  #define CHR_ROM_8K_SIZE_INDEX  5

  ubyte flags6 = header[6];
  ubyte flags7 = header[7];
  ubyte prgRam8KSize = header[8];
  ubyte flags9 = header[9];
  ubyte flags10 = header[10];

  printf("prgRomSize %u (x 16K)\n", header[PRG_ROM_16K_SIZE_INDEX]);
  printf("chrRomSize %u (x 8K)\n", header[CHR_ROM_8K_SIZE_INDEX]);
  printFlags6(flags6);
  printFlags7(flags7);
  printf("prgRamSize %u (x 8K)\n", prgRam8KSize);
  printFlags9(flags9);
  printf("flags10    0x%02x\n", flags10);

  size_t offset = 16;
  if(flags6 & FLAGS6_TRAINER_MASK) {
    printf("Cartridge load: Error: trainer mask not implemented\n");
  }

  // TODO: should the program ROM and CHR ROM be in the same malloc's buffer?

  // Create ROM memory
  prgRomSize = header[PRG_ROM_16K_SIZE_INDEX]*_16K;
  prgRom = (ubyte*)malloc(prgRomSize);
  if(!prgRom) {
    printf("Error: malloc(%u) failed (e=%d)\n", prgRomSize, errno);
    return false;
  }
  printf("Allocated %u bytes for PGR ROM\n", prgRomSize);

  chrRomSize = header[CHR_ROM_8K_SIZE_INDEX]*_8K;
  chrRom = (ubyte*)malloc(chrRomSize);
  if(!chrRom) {
    printf("Error: malloc(%u) failed (e=%d)\n", chrRomSize, errno);
    return false;
  }
  printf("Allocated %u bytes for CHR ROM\n", chrRomSize);

  // Read ROM (PRG-ROM goes to address 0x8000)
  {
    size_t bytesRead = fread(prgRom, 1, prgRomSize, file.ptr);
    if(bytesRead != prgRomSize) {
      printf("Cartridge load: Error: expected fread to return %u, but returned %u (e=%d)\n",
	     prgRomSize, bytesRead, errno);
      return false;
    }
  }
  {
    size_t bytesRead = fread(chrRom, 1, chrRomSize, file.ptr);
    if(bytesRead != chrRomSize) {
      printf("Cartridge load: Error: expected fread to return %u, but returned %u (e=%d)\n",
	     chrRomSize, bytesRead, errno);
      return false;
    }
  }
  // TODO: add sanity check to make sure the rest of the header is 0's

  // Sanity check: make sure there is not more file contents
  {
    ubyte trash;
    size_t bytesRead = fread(&trash, 1, 1, file.ptr);
    if(bytesRead != 0) {
      printf("Cartridge load: Error: expected to be at the end of the file, but there is more!\n");
      return false;
    }
  }

  return true; // success
}
