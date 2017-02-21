#pragma once

#define APU_FLAGS_ADDRESS 0x4015
#define APU_FLAG_SQUARE_1 0x01
#define APU_FLAG_SQUARE_2 0x02
#define APU_FLAG_TRIANGLE 0x04
#define APU_FLAG_NOISE    0x08
#define APU_FLAG_DMC      0x10
#define APU_FLAG_IRQ      0x10

// Returns: false on error
bool setupAudio();

extern ubyte apuStatusRegister;

struct ApuStatusRegisterDecoded
{
  bool squareWave1Enabled;
  bool squareWave2Enabled;
  bool triangleWaveEnabled;
  bool noiseEnabled;
  bool dmcEnabled;
  bool irqEnabled;
  ApuStatusRegisterDecoded() :
    squareWave1Enabled((apuStatusRegister & APU_FLAG_SQUARE_1) ? true : false),
    squareWave2Enabled ((apuStatusRegister & APU_FLAG_SQUARE_2) ? true : false),
    triangleWaveEnabled ((apuStatusRegister & APU_FLAG_TRIANGLE) ? true : false),
    noiseEnabled ((apuStatusRegister & APU_FLAG_NOISE) ? true : false),
    dmcEnabled ((apuStatusRegister & APU_FLAG_DMC) ? true : false),
    irqEnabled ((apuStatusRegister & APU_FLAG_IRQ) ? true : false)
  {
  }
};

// Addr 0-F
// 0-3 square wave 1
// 4-7 square wave 2 (identical to 1 except for frequency sweeps)
// 8-B triangle wave
// C-F noise
void ApuWriteChannelRegister(ubyte addr, ubyte value);

