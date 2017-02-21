#include <windows.h>
#include <stdio.h>
#include "common.h"

ubyte apuStatusRegister;

bool setupAudio()
{
  apuStatusRegister = 0;
  
  /*
  HWAVEOUT waveOut;
  {
    WAVEFORMATEX format;
    format.nSamplesPerSec
  }
  */
  printf("WARNING: waveout: setupAudio not implemented\n");
  return true; // success
}

void ApuWriteChannelRegister(ubyte addr, ubyte value)
{
  printf("ApuWriteChannelRegister[$%X] = $%02X not implemented\n", addr, value);
}
