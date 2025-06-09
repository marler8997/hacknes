#pragma once

#define _8K  0x2000 // 8192
#define _16K 0x4000 // 16384

// ubyte - 1 byte unsigned
typedef unsigned char ubyte;
// sbyte - 1 byte signed
typedef char sbyte;
// ushort - 2 byte unsigned
typedef unsigned short ushort;

struct Buffer
{
  ubyte* ptr;
  size_t length;
  Buffer() : ptr(NULL), length(0)
  {
  }
  Buffer(ubyte* ptr, size_t length) : ptr(ptr), length(length)
  {
  }
};

enum MirrorType {
  MIRROR_TYPE_HORIZONTAL    = 0x00,
  MIRROR_TYPE_VERTICAL      = 0x01,
  MIRROR_TYPE_NONE_USE_VRAM = 0x02,
};

extern "C" {

extern bool graphicsEnabled;

}
