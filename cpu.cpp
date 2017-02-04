#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "platform.h"
#include "cartridge.h"
#include "ppu.h"

#define ENABLE_TEST_LOG
//#define ENABLE_STACK_LOG

// TODO: define the endian functions correctly
#define LITTLE_TO_HOST_ENDIAN(x) x

Cartridge cartridge;
ushort PCReg; // Program Counter
ubyte PReg;   // Processor Status
ubyte AReg;   // Accumulator
ubyte XReg;   // Index Register X
ubyte YReg;   // Index Register Y
ubyte SPReg;  // Stack Pointer
ubyte ram[0x800];
ubyte* programRomBank8000;
ubyte* programRomBankC000;

#define STATUS_FLAG_CARRY         0x01
#define STATUS_FLAG_ZERO          0x02
#define STATUS_FLAG_NO_INTERRUPTS 0x04
#define STATUS_FLAG_DECIMAL_MODE  0x08
#define STATUS_FLAG_BREAK         0x10 // not actually a hardware flag
#define STATUS_FLAG_UNDEFINED_20  0x20
#define STATUS_FLAG_OVERFLOW      0x40
#define STATUS_FLAG_NEGATIVE      0x80


#define STATUS_MASK_ZN (STATUS_FLAG_ZERO|STATUS_FLAG_NEGATIVE)
#define STATUS_MASK_CZN (STATUS_FLAG_CARRY|STATUS_FLAG_ZERO|STATUS_FLAG_NEGATIVE)
#define STATUS_MASK_ZON (STATUS_FLAG_ZERO|STATUS_FLAG_OVERFLOW|STATUS_FLAG_NEGATIVE)
#define STATUS_MASK_CZON (STATUS_FLAG_CARRY|STATUS_FLAG_ZERO|STATUS_FLAG_OVERFLOW|STATUS_FLAG_NEGATIVE)

#define STATUS_MASK_ALL (STATUS_FLAG_CARRY|STATUS_FLAG_ZERO|STATUS_FLAG_NO_INTERRUPTS| \
                         STATUS_FLAG_DECIMAL_MODE|STATUS_FLAG_UNDEFINED_20| \
                         STATUS_FLAG_OVERFLOW|STATUS_FLAG_NEGATIVE)

const char *StatusStringTable[] = {
  "-",      "C",      "Z",      "ZC",     "I",      "IC",     "IZ",     "IZC",
  "D",      "DC",     "DZ",     "DZC",    "DI",     "DIC",    "DIZ",    "DIZC",
  "B",      "BC",     "BZ",     "BZC",    "BI",     "BIC",    "BIZ",    "BIZC",
  "BD",     "BDC",    "BDZ",    "BDZC",   "BDI",    "BDIC",   "BDIZ",   "BDIZC",

  "?",      "?C",     "?Z",     "?ZC",    "?I",     "?IC",    "?IZ",    "?IZC",
  "?D",     "?DC",    "?DZ",    "?DZC",   "?DI",    "?DIC",   "?DIZ",   "?DIZC",
  "?B",     "?BC",    "?BZ",    "?BZC",   "?BI",    "?BIC",   "?BIZ",   "?BIZC",
  "?BD",    "?BDC",   "?BDZ",   "?BDZC",  "?BDI",   "?BDIC",  "?BDIZ",  "?BDIZC",

  "O",      "C",      "Z",      "ZC",     "I",      "IC",     "IZ",     "IZC",
  "OD",     "ODC",    "ODZ",    "ODZC",   "ODI",    "ODIC",   "ODIZ",   "ODIZC",
  "OB",     "OBC",    "OBZ",    "OBZC",   "OBI",    "OBIC",   "OBIZ",   "OBIZC",
  "OBD",    "OBDC",   "OBDZ",   "OBDZC",  "OBDI",   "OBDIC",  "OBDIZ",  "OBDIZC",

  "O?",     "O?C",    "O?Z",    "O?ZC",   "O?I",    "O?IC",   "O?IZ",   "O?IZC",
  "O?D",    "O?DC",   "O?DZ",   "O?DZC",  "O?DI",   "O?DIC",  "O?DIZ",  "O?DIZC",
  "O?B",    "O?BC",   "O?BZ",   "O?BZC",  "O?BI",   "O?BIC",  "O?BIZ",  "O?BIZC",
  "O?BD",   "O?BDC",  "O?BDZ",  "O?BDZC", "O?BDI",  "O?BDIC", "O?BDIZ", "O?BDIZC",

  "N",      "NC",     "NZ",     "NZC",    "NI",     "NIC",    "NIZ",    "NIZC",
  "ND",     "NDC",    "NDZ",    "NDZC",   "NDI",    "NDIC",   "NDIZ",   "NDIZC",
  "NB",     "NBC",    "NBZ",    "NBZC",   "NBI",    "NBIC",   "NBIZ",   "NBIZC",
  "NBD",    "NBDC",   "NBDZ",   "NBDZC",  "NBDI",   "NBDIC",  "NBDIZ",  "NBDIZC",

  "N?",     "N?C",    "N?Z",    "N?ZC",   "N?I",    "N?IC",   "N?IZ",   "N?IZC",
  "N?D",    "N?DC",   "N?DZ",   "N?DZC",  "N?DI",   "N?DIC",  "N?DIZ",  "N?DIZC",
  "N?B",    "N?BC",   "N?BZ",   "N?BZC",  "N?BI",   "N?BIC",  "N?BIZ",  "N?BIZC",
  "N?BD",   "N?BDC",  "N?BDZ",  "N?BDZC", "N?BDI",  "N?BDIC", "N?BDIZ", "N?BDIZC",

  "NO",     "NC",     "NZ",     "NZC",    "NI",     "NIC",    "NIZ",    "NIZC",
  "NOD",    "NODC",   "NODZ",   "NODZC",  "NODI",   "NODIC",  "NODIZ",  "NODIZC",
  "NOB",    "NOBC",   "NOBZ",   "NOBZC",  "NOBI",   "NOBIC",  "NOBIZ",  "NOBIZC",
  "NOBD",   "NOBDC",  "NOBDZ",  "NOBDZC", "NOBDI",  "NOBDIC", "NOBDIZ", "NOBDIZC",

  "NO?",    "NO?C",   "NO?Z",   "NO?ZC",  "NO?I",   "NO?IC",  "NO?IZ",  "NO?IZC",
  "NO?D",   "NO?DC",  "NO?DZ",  "NO?DZC", "NO?DI",  "NO?DIC", "NO?DIZ", "NO?DIZC",
  "NO?B",   "NO?BC",  "NO?BZ",  "NO?BZC", "NO?BI",  "NO?BIC", "NO?BIZ", "NO?BIZC",
  "NO?BD",  "NO?BDC", "NO?BDZ", "NO?BDZC","NO?BDI", "NO?BDIC","NO?BDIZ","NO?BDIZC",
};

static size_t cycleCount = 0;
void cpuCycled()
{
  ppuStep();
  ppuStep();
  ppuStep();
  cycleCount++;
}
void cpuCycled(unsigned n)
{
  for(;n;n--) {
    ppuStep();
    ppuStep();
    ppuStep();
    cycleCount++;
  }
}

void DmaToPpu(ubyte startDividedBy256)
{
  printf("[DEBUG] PpuDma $%02X (actual address $%04X)\n",
	 startDividedBy256, startDividedBy256*256);
}

// Do not cycle the CPU
ubyte CpuReadByteForLog(ushort addr)
{
  if(addr < 0x2000) {
    // The first $2000 bytes are just a mirror of the first $800 bytes.
    // Addresses are normalized to the first $800 by masking with $7FF.
    return ram[addr & 0x7FF];
  }
  if(addr < 0x4000) {
    // The first 8 bytes are mirrored 1024 times up to 0x4000.
    return PpuIOReadForLog(addr & 0x07);
  }
  if(addr < 0x8000) {
    printf("CpuReadByte: WARNING: unmapped address %04X\n", addr);
    return 0;
  }
  if(addr < 0xC000) {
    return programRomBank8000[addr-0x8000];
  }
  return programRomBankC000[addr-0xC000];
}
ubyte CpuReadByte(ushort addr)
{
  if(addr < 0x2000) {
    // The first $2000 bytes are just a mirror of the first $800 bytes.
    // Addresses are normalized to the first $800 by masking with $7FF.
    cpuCycled();
    return ram[addr & 0x7FF];
  }
  if(addr < 0x4000) {
    // The first 8 bytes are mirrored 1024 times up to 0x4000.
    return PpuIORead(addr & 0x07);
  }
  if(addr < 0x8000) {
    printf("CpuReadByte: WARNING: unmapped address %04X\n", addr);
    return 0;
  }
  if(addr < 0xC000) {
    cpuCycled();
    return programRomBank8000[addr-0x8000];
  }
  cpuCycled();
  return programRomBankC000[addr-0xC000];
}
ushort CpuReadShort(ushort addr)
{
  if(addr < 0x2000) {
    // The first $2000 bytes are just a mirror of the first $800 bytes.
    // Addresses are normalized to the first $800 by masking with $7FF.
    cpuCycled(2);
    addr &= 0x7FF;
    return ram[addr+1] << 8 | ram[addr];
  }
  if(addr < 0x4000) {
    // The first 8 bytes are mirrored 1024 times up to 0x4000.
    addr = addr & 0x07;
    return PpuIORead(addr+1) << 8 | PpuIORead(addr);
  }
  if(addr < 0x8000) {
    printf("CpuReadShort: WARNING: unmapped address %04X\n", addr);
    return 0;
  }
  if(addr < 0xC000) {
    cpuCycled(2);
    addr -= 0x8000;
    return programRomBank8000[addr+1] << 8 | programRomBank8000[addr];
  }
  cpuCycled(2);
  addr -= 0xC000;
  return programRomBankC000[addr+1] << 8 | programRomBankC000[addr];
}
void CpuWriteByte(ushort addr, ubyte value)
{
  if(addr < 0x2000) {
    // The first $2000 bytes are just a mirror of the first $800 bytes.
    // Addresses are normalized to the first $800 by masking with $7FF.
    cpuCycled();
    ram[addr&0x7FF] = value;
  } else if(addr < 0x4000) {
    // The first 8 bytes are mirrored 1024 times up to 0x4000.
    PpuIOWrite(addr & 0x07, value);
  } else if(addr < 0x8000) {
    if(addr == 0x4014) {
      cpuCycled(512); // I think this is the right number of cycles
      DmaToPpu(value);
    } else {
      printf("CpuWriteByte: WARNING: unmapped address $%04X\n", addr);
    }
  } else {
    printf("Error: cannot write to address $%04X, because is is the cartridge ROM\n",
           addr);
    throw "error";
  }
}

void CpuWriteByteWithIntAddr(int addr, ubyte value)
{
  if(addr < 0 || addr > 0xFFFF) {
    printf("Error: code bug(line %u): invalid addr %d\n", __LINE__, addr);
    throw "code bug";
  }
  CpuWriteByte((ushort)addr, value);
}


#ifdef ENABLE_STACK_LOG
void DumpStack()
{
  for(ubyte addr = 0xFD; addr > SPReg; addr--) {
    printf(" $%02X", ram[(ushort)addr | 0x100]);
  }
}
#endif
void CpuPushStack(ubyte value)
{
  cpuCycled();
  ram[(ushort)SPReg | 0x100] = value;
  SPReg--;
#ifdef ENABLE_STACK_LOG
  printf("[DEBUG] CpuPushStack(%u):", 0xFD-SPReg);
  DumpStack();
  putchar('\n');
#endif
}
ubyte CpuPopStack()
{
  cpuCycled();
  SPReg++;
  ubyte value = ram[(ushort)SPReg | 0x100];
#ifdef ENABLE_STACK_LOG
  printf("[DEBUG] CpuPopStack(%u):", 0xFD-SPReg);
  DumpStack();
  printf(" ($%02X)\n", value);
#endif
  return value;
}

#define IRQ_RESET 0x01

#define LOG_STEP(fmt,...)
//#define LOG_STEP(fmt,...) printf(fmt"\n", ##__VA_ARGS__)
#define LOG_INSTRUCTION(fmt,...)
//#define LOG_INSTRUCTION(fmt,...) printf("[INSTRUCTION] " fmt "\n", ##__VA_ARGS__)

#define PSTATUS_FMT      "%s($%02X)"
#define PSTATUS_DIFF_FMT "%s($%02X) > %s($%02X)"

//#define PSTATUS_ARGS
#define PSTATUS_DIFF_ARGS(old) StatusStringTable[old], old, StatusStringTable[PReg], PReg

#define ASSERT_INVALID_CODE_PATH(line) do { \
    printf("Error: code bug(line %u): invalid code path\n", line); return 1; } while(0)

#define UPDATE_STATUS_ZERO_NEGATIVE(value) do {				\
    ubyte newFlags;							\
    if((ubyte)value == 0) {						\
      newFlags = STATUS_FLAG_ZERO;					\
    } else if(value & 0x80) {                                           \
      newFlags = STATUS_FLAG_NEGATIVE;					\
    } else {								\
      newFlags = 0;							\
    }									\
    ubyte oldPReg = PReg;                                               \
    PReg = newFlags | (PReg & ~STATUS_MASK_ZN);				\
    LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, PSTATUS_DIFF_ARGS(oldPReg)); \
  } while(0)

void SetFlagsFromCompare(ubyte left, ubyte right)
{
  ubyte difference = left - right;
  ubyte newFlags = 0;
  if(difference == 0) {
    newFlags |= STATUS_FLAG_ZERO;
  } else if(difference & 0x80) {
    newFlags |= STATUS_FLAG_NEGATIVE;
  }
  if(left >= right) {
    newFlags |= STATUS_FLAG_CARRY;
  }
  {
    ubyte oldPReg = PReg;
    PReg = newFlags | (PReg & ~STATUS_MASK_CZN);
    LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, PSTATUS_DIFF_ARGS(oldPReg));
  }
}
 
int runCpu()
{
  // Setup MemoryMap to point to cartridge Program ROM banks
  // - The first 2 banks are mapped to addres $8000 and $C000.
  // - If there is only 1 ROM bank, the first bank is mapped to both addresses.
  programRomBank8000 = cartridge.prgRom;
  if(cartridge.prgRomSize >= 2*_16K) {
    programRomBankC000 = cartridge.prgRom + _16K;
  } else {
    programRomBankC000 = cartridge.prgRom; // mirror the first bank
  }

  ubyte irqFlags;
  bool jammed;

  irqFlags = IRQ_RESET;

#ifdef ENABLE_TEST_LOG
  #define MAX_INFO_STRING 27
  
  char testOpArgString[6];
  #define TEST_LOG_SET_NO_ARGS() testOpArgString[0] = '\0'
  #define TEST_LOG_SET_1_ARG()   sprintf(testOpArgString, "%02X", opParam)
  #define TEST_LOG_SET_2_ARGS()  sprintf(testOpArgString, "%02X %02X", opParam, opParam2)

  char testInfoString[MAX_INFO_STRING+1];
  #define TEST_LOG_SET_INFO(fmt,...) sprintf(testInfoString, fmt,##__VA_ARGS__)

  ushort savePCReg;
  ushort saveScanlineCycle;
  #define TEST_PROC_STATUS_FMT "A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3u\n"
  #define TEST_PROC_STATUS_ARGS AReg, XReg, YReg, PReg, SPReg, saveScanlineCycle
  #define TEST_LOG_0_ARGS(name) printf("%04X  %02X       %4s %-27s " TEST_PROC_STATUS_FMT, \
                                       savePCReg, op, name, testInfoString, TEST_PROC_STATUS_ARGS);
  #define TEST_LOG_1_ARG(name) printf("%04X  %02X %02X    %4s %-27s "   TEST_PROC_STATUS_FMT, \
                                      savePCReg, op, opParam, name, testInfoString, TEST_PROC_STATUS_ARGS);
  #define TEST_LOG_2_ARGS(name) printf("%04X  %02X %02X %02X %4s %-27s " TEST_PROC_STATUS_FMT, \
                                       savePCReg, op, opParam, opParam2, name, testInfoString, TEST_PROC_STATUS_ARGS);
  #define TEST_LOG_PRESET_ARGS(name) printf("%04X  %02X %-5s %4s %-27s " TEST_PROC_STATUS_FMT, \
                                            savePCReg, op, testOpArgString, name, testInfoString, TEST_PROC_STATUS_ARGS);
#else
  #define TEST_LOG_SET_NO_ARGS()
  #define TEST_LOG_SET_1_ARG()
  #define TEST_LOG_SET_2_ARGS()
  #define TEST_LOG_SET_INFO(fmt,...)
  
  #define TEST_LOG_0_ARGS(name)
  #define TEST_LOG_1_ARG(name)
  #define TEST_LOG_2_ARGS(name)
  #define TEST_LOG_PRESET_ARGS(name)
#endif

#define BRANCH(name, cond) do {                                         \
    PCReg++; /* consumes opParam */                                     \
    TEST_LOG_SET_INFO("$%04X", PCReg + (sbyte)opParam);                 \
    TEST_LOG_1_ARG(name);                                               \
    if (cond) {                                                         \
      PCReg += (sbyte)opParam;                                          \
      cpuCycled();                                                      \
      /*TODO: figure out how to check if it's a new page*/              \
      /*if(newPage) cpuCycled(2);*/                                     \
      LOG_INSTRUCTION(name " %d (JUMPING! PC=$%04X)", (sbyte)opParam, PCReg); \
    } else {                                                            \
      LOG_INSTRUCTION(name " %d (NOT JUMPING)", (sbyte)opParam);        \
    }                                                                   \
  } while(0)
  
#define SET_PFLAG(name,flag) do {                                       \
    TEST_LOG_SET_INFO("");                                              \
    TEST_LOG_0_ARGS(name);                                              \
    {                                                                   \
      ubyte oldPReg = PReg;                                             \
      PReg |= flag;                                                     \
      LOG_INSTRUCTION(name "  Status " PSTATUS_DIFF_FMT, PSTATUS_DIFF_ARGS(oldPReg)); \
    }                                                                   \
  } while(0)
#define CLEAR_PFLAG(name,flag) do {                                     \
    TEST_LOG_SET_INFO("");                                              \
    TEST_LOG_0_ARGS(name);                                              \
    {                                                                   \
      ubyte oldPReg = PReg;                                             \
      PReg &= ~flag;                                                    \
      LOG_INSTRUCTION(name "  Status " PSTATUS_DIFF_FMT, PSTATUS_DIFF_ARGS(oldPReg)); \
    }                                                                   \
  } while(0)
  
  for(size_t i = 0; /*i < 100000*/; i++) {

    // Handle interrupts
    // TODO: do I ignore these if STATUS_FLAG_NO_INTERRUPTS is set?
    if(irqFlags) {
      if(irqFlags & IRQ_RESET) {
	LOG_INSTRUCTION("IRQ_RESET");

        PCReg = LITTLE_TO_HOST_ENDIAN(*(ushort*)&programRomBankC000[0xFFFC-0xC000]);
        printf("PCReg initialized to $%04X\n", PCReg);

	jammed = false;
	PReg = STATUS_FLAG_NO_INTERRUPTS;
	irqFlags &= ~IRQ_RESET;
	SPReg = 0xFD; // TODO: am I supposed to set this here? FECU doesn't?
        PReg = STATUS_FLAG_UNDEFINED_20 | STATUS_FLAG_NO_INTERRUPTS;
	// TODO: initialize the mapper
      }
    }

#ifdef ENABLE_TEST_LOG
    savePCReg = PCReg;
    saveScanlineCycle = ppuState.scanlineCycle;
#endif
    
    // The CPU always reads 2 bytes during the fetch cycle
    ubyte op      = CpuReadByte(PCReg++);
    ubyte opParam = CpuReadByte(PCReg);
    ubyte opParam2;

    LOG_STEP("--------------------------------------------------------------------------------");
    LOG_STEP("CpuTick %u, Cycle %u, PC $%04X, OP $%02X, PARAM $%02X, Status %s($%02X)",
	     i, cycleCount, PCReg, op, opParam, StatusStringTable[PReg], PReg);

    switch(op & 0x03) {
      //
      // op = ------00
      //
    case 0:
      switch(op >> 2) {
      case 0:  // $00
        printf("Error: op $%02X not implemented\n", op); return 1;
        break;
      case 1:  // $04 *NOP (Instruction not supported)
	PCReg++; // consumes opParam
	TEST_LOG_SET_INFO("$%02X = %02X", opParam, CpuReadByteForLog(opParam));
	TEST_LOG_1_ARG("*NOP");
	cpuCycled();
        break;
      case 2:  // $08 PHP - Push Processor Status
        TEST_LOG_SET_INFO("");
        TEST_LOG_0_ARGS("PHP");
        // Always set flag 0x20 when pushing to the stack (don't know why).
        // Set the BRK flag to indicate the processor status was pushed to
        // the stack instruction (BRK or PHP) and not an IRQ.
        CpuPushStack(STATUS_FLAG_UNDEFINED_20 | STATUS_FLAG_BREAK | PReg);
        break;
      case 3: // $0C *NOP (Instruction not supported)
	{
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2; // consumes both params
	  ushort addr = ((ushort)opParam2) << 8 | opParam;
	  TEST_LOG_SET_INFO("$%04X = %02X", addr, CpuReadByteForLog(addr));
	  TEST_LOG_2_ARGS("*NOP");
	  cpuCycled();
	}
        break;
      case 4:  // $10 BPL - Branch is Positive
        BRANCH("BPL", !(PReg & STATUS_FLAG_NEGATIVE));
        break;
      NOP_ZEROPAGE_X:
      case 5:  // $14 *NOP (Instruction not supported)
	{
	  PCReg++; // consumes opParam
	  ubyte addr = XReg + opParam;
	  TEST_LOG_SET_INFO("$%02X,X @ %02X = %02X", opParam, addr, CpuReadByteForLog(addr));
	  TEST_LOG_1_ARG("*NOP");
	  cpuCycled(2);
	}
        break;
      case 6:  // $18 CLC - Clear Carry Flag
        CLEAR_PFLAG("CLC", STATUS_FLAG_CARRY);
        break;
      NOP_ABSOLUTE_X:
      case 7:  // $1C *NOP (Instruction not supported)
	opParam2 = CpuReadByte(PCReg+1);
	PCReg += 2; // consume both op params
	{
	  ushort addr = (ushort)opParam2 << 8 | (ushort)opParam;
	  ushort addrPlusX = addr + (ushort)XReg;
	  TEST_LOG_SET_INFO("$%04X,X @ %04X = %02X", addr, addrPlusX, CpuReadByteForLog(addrPlusX));
	  TEST_LOG_2_ARGS("*NOP");
	  cpuCycled();
	  if( ( (ushort)opParam+(ushort)XReg ) & 0x100) {
	    cpuCycled(); // extra cycle if page boundary crossed
	  }
	}
        break;
      case 8:  // $20 JSR - Jump to Subroutine
        {
          opParam2 = CpuReadByte(PCReg+1);
          ushort immediate = (ushort)opParam2 << 8 | (ushort)opParam;
          PCReg += 2; // op consumes 2 bytes
          TEST_LOG_SET_INFO("$%04X", immediate);
          TEST_LOG_2_ARGS("JSR");
          {
            ushort pushToStack = PCReg-1;
            CpuPushStack(pushToStack >> 8);
            CpuPushStack(pushToStack);
          }
          PCReg = immediate;
          cpuCycled(); // need another cycle for some reason
        }
        break;
      case 9:  // $24 BIT - Bit Test
        PCReg++; // consume opParam
        {
          ubyte M = CpuReadByte(opParam);
          TEST_LOG_SET_INFO("$%02X = %02X", opParam, M);
          TEST_LOG_1_ARG("BIT");
          ubyte newFlags = 0xC0 & M;
          if((M & AReg) == 0) {
            newFlags |= STATUS_FLAG_ZERO;
          }
          ubyte oldPReg = PReg;
          PReg = newFlags | (PReg & ~STATUS_MASK_ZON);
        }
        break;
      case 10: // $28 PLP - Pull Processor Status
        TEST_LOG_SET_INFO("");
        TEST_LOG_0_ARGS("PLP");
        // Not sure why, but I'm supposed to set flag 0x20 whenever
        // the status register is loaded from memory
        PReg = STATUS_FLAG_UNDEFINED_20 | (CpuPopStack() & STATUS_MASK_ALL);
        cpuCycled();
        break;
      case 11: // $2C BIT - Bit Test
	{
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2; // consumes 2 bytes
	  ushort addr = ((ushort)opParam2) << 8 | opParam;
	  TEST_LOG_SET_INFO("$%04X = %02X", addr, CpuReadByteForLog(addr));
	  TEST_LOG_2_ARGS("BIT");
	  ubyte M = CpuReadByte(addr);
          ubyte newFlags = 0xC0 & M;
          if((M & AReg) == 0) {
            newFlags |= STATUS_FLAG_ZERO;
          }
          ubyte oldPReg = PReg;
          PReg = newFlags | (PReg & ~STATUS_MASK_ZON);
        }
        break;
      case 12: // $30
        BRANCH("BMI", PReg & STATUS_FLAG_NEGATIVE);
        break;
      case 13: // $34
	goto NOP_ZEROPAGE_X;
      case 14: // $38 SEC - Set Carry Flag
        SET_PFLAG("SEC", STATUS_FLAG_CARRY);
        break;
      case 15: // $3C
	goto NOP_ABSOLUTE_X;
      case 16: // $40 RTI - Return from Interrupt
	TEST_LOG_SET_INFO("");
	TEST_LOG_0_ARGS("RTI");
        // Not sure why, but I'm supposed to set flag 0x20 whenever
        // the status register is loaded from memory
        PReg = STATUS_FLAG_UNDEFINED_20 | (CpuPopStack() & STATUS_MASK_ALL);
	{
	  PCReg = CpuPopStack();
	  PCReg |= CpuPopStack() << 8;
	}
        cpuCycled();
        break;
      case 17: // $44 *NOP (Instruction not supported)
	PCReg++; // consumes opParam
	TEST_LOG_SET_INFO("$%02X = %02X", opParam, CpuReadByteForLog(opParam));
	TEST_LOG_1_ARG("*NOP");
	cpuCycled();
        break;
      case 18: // $48 PHA - Push Accumulator
        TEST_LOG_SET_INFO("");
        TEST_LOG_0_ARGS("PHA");
        CpuPushStack(AReg);
        break;
      case 19: // $4C JMP - Jump
        {
          opParam2 = CpuReadByte(PCReg+1);
          ushort immediate = (ushort)opParam2 << 8 | (ushort)opParam;
          PCReg += 2; // op consumes 2 bytes
          TEST_LOG_SET_INFO("$%04X", immediate);
          TEST_LOG_2_ARGS("JMP");
          PCReg = immediate;
        }
        break;
      case 20: // $50
        BRANCH("BVC", !(PReg & STATUS_FLAG_OVERFLOW));
        break;
      case 21: // $54
	goto NOP_ZEROPAGE_X;
      case 22: // $58
        printf("Error: op $%02X not implemented\n", op); return 1;
        break;
      case 23: // $5C
	goto NOP_ABSOLUTE_X;
      case 24: // $60 RTS - Return from Subroutine
        {
          TEST_LOG_SET_INFO("");
          TEST_LOG_0_ARGS("RTS");
          PCReg = CpuPopStack();
          PCReg |= ((ushort)CpuPopStack()) << 8;
          PCReg++;
          cpuCycled(2);
        }
        break;
      case 25:  // $64 *NOP (Instruction not supported)
	PCReg++; // consumes opParam
	TEST_LOG_SET_INFO("$%02X = %02X", opParam, CpuReadByteForLog(opParam));
	TEST_LOG_1_ARG("*NOP");
	cpuCycled();
        break;
      case 26: // $68 PLA - Pull Accumulator
        TEST_LOG_SET_INFO("");
        TEST_LOG_0_ARGS("PLA");
        AReg = CpuPopStack();
        UPDATE_STATUS_ZERO_NEGATIVE(AReg);
        cpuCycled();
        break;
      case 27: // $6C JMP - Jump
        {
          opParam2 = CpuReadByte(PCReg+1);
          ushort immediate = (ushort)opParam2 << 8 | (ushort)opParam;
          PCReg += 2; // op consumes 2 bytes

	  ushort upperByteAddr;
	  if(opParam == 0xFF) { // Reproduce the 6502 hardware bug
	    upperByteAddr = (immediate + 1) & 0xFF00;
	    TEST_LOG_SET_INFO("($%04X) = %04X", immediate, upperByteAddr);
	    TEST_LOG_2_ARGS("JMP");
	    // TODO: HUH?
	    // This looks like a bug in Nintendemulator.  I need
	    // to confirm this is what the bug actually does.
	    PCReg = upperByteAddr;
	    // TODO: I also think this cpuCycled is probably supposed
	    //       to be done in BOTH cases, not sure though.
	    cpuCycled(2);
	  } else {
	    upperByteAddr = immediate + 1;
	    PCReg = ((ushort)CpuReadByte(upperByteAddr)) << 8 | (ushort)CpuReadByte(immediate);
	    TEST_LOG_SET_INFO("($%04X) = %04X", immediate, PCReg);
	    TEST_LOG_2_ARGS("JMP");
	  }
        }
        break;
      case 28: // $70 BVS - Branch if Overflow Set
        BRANCH("BVS", PReg & STATUS_FLAG_OVERFLOW);
        break;
      case 29: // $74
	goto NOP_ZEROPAGE_X;
      case 30: // $78 SEI - Set Interrupt Disable
        SET_PFLAG("SEI", STATUS_FLAG_NO_INTERRUPTS);
        break;
      case 31: // $7C
	goto NOP_ABSOLUTE_X;
      case 32: // $80 NOP (Instruction not supported)
	PCReg++; // consumes opParam
	TEST_LOG_SET_INFO("#$%02X", opParam);
	TEST_LOG_1_ARG("*NOP");
        break;
      case 33: // $84 STY - Store Y Register: ZeroPage
        PCReg++; // consumes opParam
        TEST_LOG_SET_INFO("$%02X = %02X", opParam, CpuReadByteForLog(opParam));
        TEST_LOG_1_ARG("STY");
        CpuWriteByte(opParam, YReg);
        break;
      case 34: // $88 DEY - Decrement Y Register
        TEST_LOG_SET_INFO("");
        TEST_LOG_0_ARGS("DEY");
        YReg--;
        UPDATE_STATUS_ZERO_NEGATIVE(YReg);
        break;
      case 35: // $8C STY - Store Y Register : Absolute
	{
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2; // consumes 2 bytes
	  ushort addr = ((ushort)opParam2) << 8 | opParam;
	  TEST_LOG_SET_INFO("$%04X = %02X", addr, CpuReadByteForLog(addr));
	  TEST_LOG_2_ARGS("STY");
	  CpuWriteByte(addr, YReg);
	}
        break;
      case 36: // $90 BCC - Branch if Carry Clear
        BRANCH("BCC", !(PReg & STATUS_FLAG_CARRY));
        break;
      case 37: // $94 STY - Store Y Register : ZeroPage,X
        PCReg++; // consumes opParam
	{
	  ubyte addr = opParam + XReg;
	  TEST_LOG_SET_INFO("$%02X,X @ %02X = %02X", opParam, addr, CpuReadByteForLog(addr));
	  TEST_LOG_1_ARG("STY");
	  CpuWriteByte(addr, YReg);
	  cpuCycled();
	}
        break;
      case 38: // $98 TYA - Transfer Y to Accumulator
        TEST_LOG_SET_INFO("");
        TEST_LOG_0_ARGS("TYA");
        AReg = YReg;
        UPDATE_STATUS_ZERO_NEGATIVE(AReg);
        break;
      case 39: // $9C
        printf("Error: op $%02X not implemented\n", op); return 1;
        break;
      case 40: // $A0 LDY - Load Y Register : Immediate
        PCReg++; // consumes opParam
        TEST_LOG_SET_INFO("#$%02X", opParam);
        TEST_LOG_1_ARG("LDY");
        YReg = opParam;
        UPDATE_STATUS_ZERO_NEGATIVE(YReg);
        break;
      case 41: // $A4 LDY - Load Y Register : ZeroPage
        PCReg++; // consumes opParam
        TEST_LOG_SET_INFO("$%02X = %02X", opParam, CpuReadByteForLog(opParam));
        TEST_LOG_1_ARG("LDY");
        YReg = CpuReadByte(opParam);
        UPDATE_STATUS_ZERO_NEGATIVE(YReg);
        break;
      case 42: // $A8 TAY - Transfer Accumulator to Y
        TEST_LOG_SET_INFO("");
        TEST_LOG_0_ARGS("TAY");
        YReg = AReg;
        UPDATE_STATUS_ZERO_NEGATIVE(YReg);
        break;
      case 43: // $AC LDY - Load Y Register : Absolute
	{
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2; // consumes 2 bytes
	  ushort addr = ((ushort)opParam2) << 8 | opParam;
	  TEST_LOG_SET_INFO("$%04X = %02X", addr, CpuReadByteForLog(addr));
	  TEST_LOG_2_ARGS("LDY");
	  YReg = CpuReadByte(addr);
	  UPDATE_STATUS_ZERO_NEGATIVE(YReg);
	}
        break;
      case 44: // $B0 BCS - Branch if Carry Set
        BRANCH("BCS", PReg & STATUS_FLAG_CARRY);
        break;
      case 45: // $B4 LDY - Load Y Register : ZeroPage,X
	PCReg++; // consumes opParam
	{
	  ubyte addr = opParam + XReg;
	  TEST_LOG_SET_INFO("$%02X,X @ %02X = %02X", opParam, addr, CpuReadByteForLog(addr));
	  TEST_LOG_1_ARG("LDY");
	  YReg = CpuReadByte(addr);
	  UPDATE_STATUS_ZERO_NEGATIVE(YReg);
	  cpuCycled();
	}
        break;
      case 46: // $B8 CLV - Clear oVerflow Flag
        CLEAR_PFLAG("CLV", STATUS_FLAG_OVERFLOW);
        break;
      case 47: // $BC LDY - Load Y Register : Absolute,X
	opParam2 = CpuReadByte(PCReg+1);
	PCReg += 2; // consume 2 params
	{
	  ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
	  ushort addrOfM = (ushort)XReg + immediate;
	  ubyte M = CpuReadByte(addrOfM);
	  TEST_LOG_SET_INFO("$%04X,X @ %04X = %02X", immediate, addrOfM, M);
	  TEST_LOG_2_ARGS("LDY");
	  YReg = M;
	  UPDATE_STATUS_ZERO_NEGATIVE(YReg);
	  if(((ushort)opParam + (ushort)XReg) & 0x100) {
	    cpuCycled(); // cycle if on page boundary
	  }
	}
        break;
      case 48: // $C0 CPY - Compare Y Register: Immediate
        PCReg++; // consumes opParam
        TEST_LOG_SET_INFO("#$%02X", opParam);
        TEST_LOG_1_ARG("CPY");
        SetFlagsFromCompare(YReg, opParam);
        break;
      case 49: // $C4 CPY - Compare Y Register: ZeroPage
        PCReg++; // consumes opParam
        TEST_LOG_SET_INFO("$%02X = %02X", opParam, CpuReadByteForLog(opParam));
        TEST_LOG_1_ARG("CPY");
        SetFlagsFromCompare(YReg, CpuReadByte(opParam));
        break;
      case 50: // $C8 INY - Increment Y Register
        TEST_LOG_SET_INFO("");
        TEST_LOG_0_ARGS("INY");
        YReg++;
        UPDATE_STATUS_ZERO_NEGATIVE(YReg);
        break;
      case 51: // $CC CPY - Compare Y Register : Absolute
	{
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2; // consumes 2 bytes
	  ushort addr = ((ushort)opParam2) << 8 | opParam;
	  TEST_LOG_SET_INFO("$%04X = %02X", addr, CpuReadByteForLog(addr));
	  TEST_LOG_2_ARGS("CPY");
	  SetFlagsFromCompare(YReg, CpuReadByte(addr));
	}
        break;
      case 52: // $D0 BNE - Branch if Not Equal
        BRANCH("BNE", !(PReg & STATUS_FLAG_ZERO));
        break;
      case 53: // $D4
	goto NOP_ZEROPAGE_X;
      case 54: // $D8 CLD - Clear Decimal Mode
        CLEAR_PFLAG("CLD", STATUS_FLAG_DECIMAL_MODE);
        break;
      case 55: // $DC
	goto NOP_ABSOLUTE_X;
      case 56: // $E0 CPX - Compare X Register: Immediate
        PCReg++; // consumes opParam
        TEST_LOG_SET_INFO("#$%02X", opParam);
        TEST_LOG_1_ARG("CPX");
        SetFlagsFromCompare(XReg, opParam);
        break;
      case 57: // $E4 CPX - Compare X Register: ZeroPage
        PCReg++; // consumes opParam
        TEST_LOG_SET_INFO("$%02X = %02X", opParam, CpuReadByteForLog(opParam));
        TEST_LOG_1_ARG("CPX");
        SetFlagsFromCompare(XReg, CpuReadByte(opParam));
        break;
      case 58: // $E8 INX - Increment X Register
        TEST_LOG_SET_INFO("");
        TEST_LOG_0_ARGS("INX");
        XReg++;
        UPDATE_STATUS_ZERO_NEGATIVE(XReg);
        break;
      case 59: // $EC CPX - Compare X Register : Absolute
	{
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2; // consumes 2 bytes
	  ushort addr = ((ushort)opParam2) << 8 | opParam;
	  TEST_LOG_SET_INFO("$%04X = %02X", addr, CpuReadByteForLog(addr));
	  TEST_LOG_2_ARGS("CPX");
	  SetFlagsFromCompare(XReg, CpuReadByte(addr));
	}
        break;
      case 60: // $F0 BEQ - Branch if Equal
        BRANCH("BEQ", PReg & STATUS_FLAG_ZERO);
        break;
      case 61: // $F4
	goto NOP_ZEROPAGE_X;
      case 62: // $F8 SED - Set Decimal Flag
        SET_PFLAG("SED", STATUS_FLAG_DECIMAL_MODE);
        break;
      case 63: // $FC
	goto NOP_ABSOLUTE_X;
      }
      break;
      //
      // op = ------01
      //
    case 1:

      if((op & 0xE0) == 0x80) { // 100----- STA (Special case for cc=01, the only instruction that writes to memory)

	switch((op>>2)&0x07) {
	case 0: // ---000-- $81 STA (ZeroPage,X)
	  PCReg++; // consumes opParam
	  {
	    ubyte addrOfAddr = XReg + opParam;
	    ushort addrOfM = CpuReadByte((addrOfAddr+1)&0xFF) << 8 | CpuReadByte(addrOfAddr);
	    TEST_LOG_SET_INFO("($%02X,X) @ %02X = %04X = %02X",
			      opParam, addrOfAddr, addrOfM, CpuReadByteForLog(addrOfM));
	    TEST_LOG_1_ARG("STA");
	    CpuWriteByte(addrOfM, AReg);
	    LOG_INSTRUCTION("ZeroPage,X  X($%02X)+$%02X is %u", XReg, opParam, AReg);
	    cpuCycled(); // needs another cycle for some reason
	  }
	  break;
	case 1: // ---001-- $85 STA ZeroPage
	  PCReg++; // op consumes the param
          TEST_LOG_SET_INFO("$%02X = %02X", opParam, ram[opParam]);
          TEST_LOG_1_ARG("STA");
	  LOG_INSTRUCTION("STA $%02x", opParam);
	  CpuWriteByte(opParam, AReg);
	  break;
	case 2: // ---010-- $89 STA Immediate
	  printf("Error: invalid op $%02X\n, op (STA, immediate)\n", op);
	  return 1;
	case 3: // ---011-- $8D STA Absolute
          opParam2 = CpuReadByte(PCReg+1);
	  {
	    ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
	    PCReg += 2; // op consumes 2 bytes
	    TEST_LOG_SET_INFO("$%04X = %02X", immediate, CpuReadByteForLog(immediate));
            TEST_LOG_2_ARGS("STA");
	    LOG_INSTRUCTION("STA $%04x", immediate);
	    CpuWriteByte(immediate, AReg);
	  }
	  break;
	case 4: // ---100-- $91 STA (ZeroPage),Y
	  PCReg++; // consumes opParam
	  {
	    ubyte addrLowerByte = CpuReadByte(opParam);
	    ushort indirectAddr = (ushort)CpuReadByte((opParam+1)&0xFF) << 8 | addrLowerByte;
	    ushort addrOfM = (ushort)YReg + indirectAddr;

	    TEST_LOG_SET_INFO("($%02X),Y = %04X @ %04X = %02X",
			      opParam, indirectAddr, addrOfM, CpuReadByteForLog(addrOfM));
	    TEST_LOG_1_ARG("STA");
	    cpuCycled();
	    // Add a Cycle if YReg + lower address byte has a carry
	    if(((ushort)YReg + (ushort)addrLowerByte) & 0x100) {
	      cpuCycled();
	    }
	    CpuWriteByte(addrOfM, AReg);
	  }
	  break;
	case 5: // ---101-- $95 STA ZeroPage,X
	  PCReg++; // consumes opParam
	  {
	    ubyte addr = XReg + opParam;
	    TEST_LOG_SET_INFO("$%02X,X @ %02X = %02X", opParam, addr, CpuReadByteForLog(addr));
	    TEST_LOG_1_ARG("STA");
	    CpuWriteByte(addr, AReg);
	    cpuCycled(); // needs another cycle for some reason
	  }
	  break;
	case 6: // ---110-- $99 STA Absolute,Y
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2;
	  {
	    ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
	    ushort addrOfM = (ushort)YReg + immediate;
	    TEST_LOG_SET_INFO("$%04X,Y @ %04X = %02X",
			      immediate, addrOfM, CpuReadByteForLog(addrOfM));
	    TEST_LOG_2_ARGS("STA");
	    CpuWriteByte(addrOfM, AReg);
	    cpuCycled();
	  }
	  break;
	case 7: // ---111-- $9D STA Absolute,X
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2;
	  {
	    ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
	    ushort addrOfM = (ushort)XReg + immediate;
	    TEST_LOG_SET_INFO("$%04X,X @ %04X = %02X",
			      immediate, addrOfM, CpuReadByteForLog(addrOfM));
	    TEST_LOG_2_ARGS("STA");
	    CpuWriteByte(addrOfM, AReg);
	    //if(((ushort)opParam + (ushort)XReg) & 0x100) {
	    cpuCycled();
	    //}
	  }
	  break;
	}

      } else {

	ubyte M;
	
	switch((op >> 2) & 0x07) {
	case 0: // 000 (ZeroPage,X)
	  PCReg++; // consumes opParam
	  TEST_LOG_SET_1_ARG();
	  {
	    ubyte addrOfAddr = XReg + opParam;
	    ushort addrOfM = CpuReadByte((addrOfAddr+1)&0xFF) << 8 | CpuReadByte(addrOfAddr);
	    M = CpuReadByte(addrOfM);
	    TEST_LOG_SET_INFO("($%02X,X) @ %02X = %04X = %02X",
			      opParam, addrOfAddr, addrOfM, M);
	    LOG_INSTRUCTION("ZeroPage,X  X($%02X)+$%02X is %u", XReg, opParam, M);
	    cpuCycled(); // needs another cycle for some reason
	  }
	  break;
	case 1: // 001 ZeroPage
	  PCReg++; // consumes opParam
          TEST_LOG_SET_1_ARG();
	  M = CpuReadByte(opParam);
	  TEST_LOG_SET_INFO("$%02X = %02X", opParam, M);
	  LOG_INSTRUCTION("ZeroPage $%02X is %u", opParam, M);
	  break;
	case 2: // 010 Immediate
	  PCReg++; // consumes opParam
          TEST_LOG_SET_1_ARG();
	  M = opParam;
          TEST_LOG_SET_INFO("#$%02X", opParam);
          LOG_INSTRUCTION("#immediate is $%02X", M);
	  break;
	case 3: // 011 Absolute
          opParam2 = CpuReadByte(PCReg+1);
          {
	    ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
            PCReg += 2;
            TEST_LOG_SET_2_ARGS();
            M = CpuReadByte(immediate);
            TEST_LOG_SET_INFO("$%04X = %02X", immediate, M);
            LOG_INSTRUCTION("absolute $%04X is %u", immediate, M);
          }
	  break;
	case 4: // 100 (ZeroPage),Y
	  PCReg++; // consumes opParam
	  TEST_LOG_SET_1_ARG();
	  {
	    ubyte addrLowerByte = CpuReadByte(opParam);
	    ushort indirectAddr = (ushort)CpuReadByte((opParam+1)&0xFF) << 8 | addrLowerByte;
	    ushort addrOfM = (ushort)YReg + indirectAddr;
	    M = CpuReadByte(addrOfM);
	    TEST_LOG_SET_INFO("($%02X),Y = %04X @ %04X = %02X",
			      opParam, indirectAddr, addrOfM, M);
	    // Add a Cycle if YReg + lower address byte has a carry
	    if(((ushort)YReg + (ushort)addrLowerByte) & 0x100) {
	      cpuCycled();
	    }
	  }
	  break;
	case 5: // 101 ZeroPage,X
	  PCReg++; // consumes opParam
	  {
	    ubyte addr = XReg + opParam;
	    TEST_LOG_SET_1_ARG();
	    TEST_LOG_SET_INFO("$%02X,X @ %02X = %02X", opParam, addr, CpuReadByteForLog(addr));
	    M = CpuReadByte(addr);
	    LOG_INSTRUCTION("ZeroPage,X  X($%02X)+$%02X is %u", XReg, opParam, M);
	    cpuCycled(); // needs another cycle for some reason
	  }
	  break;
	case 6: // 110 Absolute,Y
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2;
	  TEST_LOG_SET_2_ARGS();
	  {
	    ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
	    ushort addrOfM = (ushort)YReg + immediate;
	    M = CpuReadByte(addrOfM);
	    TEST_LOG_SET_INFO("$%04X,Y @ %04X = %02X",
			      immediate, addrOfM, M);
	    LOG_INSTRUCTION("absolute,Y(%u) is $%04X (%u)", YReg, M, M);
	    if(((ushort)YReg+(ushort)opParam) & 0x100) {
	      cpuCycled();
	    }
	    break;
	  }
	  break;
	case 7: // 111 Absolute,X
	  opParam2 = CpuReadByte(PCReg+1);
	  PCReg += 2;
	  TEST_LOG_SET_2_ARGS();
	  {
	    ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
	    ushort addrOfM = (ushort)XReg + immediate;
	    M = CpuReadByte(addrOfM);
	    TEST_LOG_SET_INFO("$%04X,X @ %04X = %02X",
			      immediate, addrOfM, M);
	    LOG_INSTRUCTION("absolute,X(%u) is $%04X (%u)", XReg, M, M);
	    if(((ushort)XReg+(ushort)opParam) & 0x100) {
	      cpuCycled();
	    }
	    break;
	  }
	}

	switch(op >> 5) {
	case 0: // ---000-- ORA
          TEST_LOG_PRESET_ARGS("ORA");
          AReg = AReg | M;
          UPDATE_STATUS_ZERO_NEGATIVE(AReg);
	  break;
	case 1: // ---001-- AND
          TEST_LOG_PRESET_ARGS("AND");
          AReg = AReg & M;
          UPDATE_STATUS_ZERO_NEGATIVE(AReg);
	  break;
	case 2: // ---010-- EOR - Exclusive OR
          TEST_LOG_PRESET_ARGS("EOR");
          AReg = AReg ^ M;
          UPDATE_STATUS_ZERO_NEGATIVE(AReg);
	  break;
	case 3: // ---011-- ADC - Add with Carry
          TEST_LOG_PRESET_ARGS("ADC");
	  {
            ushort totalUShort = (ushort)AReg + (ushort)M + (ushort)(PReg & STATUS_FLAG_CARRY);
            ubyte totalUByte = totalUShort;
	    ubyte newFlags = 0;
	    if(totalUByte == 0) {
	      newFlags |= STATUS_FLAG_ZERO;
	    } else if(totalUByte & 0x80) {
	      newFlags |= STATUS_FLAG_NEGATIVE;
	    }
            newFlags |= (totalUShort >> 8) & STATUS_FLAG_CARRY;
            // Overflow:
            // (A && B && !C) || ( !A &&  !B && C)
            // TODO: there might be a better way to do this
            if(
               (  (AReg & 0x80) &&  (M & 0x80) && !(totalUByte & 0x80) ) ||
               ( !(AReg & 0x80) && !(M & 0x80) &&  (totalUByte & 0x80) ) ) {
	      newFlags |= STATUS_FLAG_OVERFLOW;
	    }
	    {
	      ubyte oldPReg = PReg;
	      PReg = newFlags | (PReg & ~STATUS_MASK_CZON);
              //LOG_INSTRUCTION("ADC %u (carry=%u) Status 0x%02x > 0x%02x", M, saveCarry, oldPReg, PReg);
	    }
	    AReg = totalUByte;
	  }
	  break;
	case 4: // ---100-- STA
          ASSERT_INVALID_CODE_PATH(__LINE__);
	  break;
	case 5: // ---101-- $A1, $A5, $A9, $AD, $B1, $B5, $B9, $BD LDA
          TEST_LOG_PRESET_ARGS("LDA");
	  AReg = M;
	  LOG_INSTRUCTION("LDA A = %u", AReg);
	  UPDATE_STATUS_ZERO_NEGATIVE(AReg);
	  break;
	case 6: // ---110-- CMP - Compare
          TEST_LOG_PRESET_ARGS("CMP");
          SetFlagsFromCompare(AReg, M);
	  break;
	case 7: // ---111-- SBC
          TEST_LOG_PRESET_ARGS("SBC");
          {
            ubyte rightHandSide =
              (~M+1) + // 2's complement of M
              ((PReg & STATUS_FLAG_CARRY) + 0xFF); // carry_flag - 1 (note: -1 == 0xFF)
            ushort totalUShort = (ushort)AReg + (ushort)rightHandSide;
            ubyte totalUByte = totalUShort;
            ubyte newFlags = 0;
	    if(totalUByte == 0) {
	      newFlags |= STATUS_FLAG_ZERO;
	    } else if(totalUByte & 0x80) {
	      newFlags |= STATUS_FLAG_NEGATIVE;
	    }
            newFlags |= (totalUShort >> 8) & STATUS_FLAG_CARRY;
            // Overflow:
            // (A && B && !C) || ( !A &&  !B && C)
            // TODO: there might be a better way to do this
            if(
               (  (AReg & 0x80) &&  (rightHandSide & 0x80) && !(totalUByte & 0x80) ) ||
               ( !(AReg & 0x80) && !(rightHandSide & 0x80) &&  (totalUByte & 0x80) ) ) {
	      newFlags |= STATUS_FLAG_OVERFLOW;
	    }
	    {
	      ubyte oldPReg = PReg;
	      PReg = newFlags | (PReg & ~STATUS_MASK_CZON);
              LOG_INSTRUCTION("SBC Status 0x%02x > 0x%02x", oldPReg, PReg);
	    }
            AReg = totalUByte;
          }
	  break;
	}
      }
      break;
      //
      // op = ------10
      //
    case 2:

      if((op & 0xE0) == 0x80) { // 100----- STX (Special case for cc=10, the only instruction that does not read from memory)
	switch( ( op >> 2 ) & 0x07) {
	case 0: // ---000-- $82 STX - Store X Register : Immediate
	  printf("Error: invalid op $%02X\n, op (STA, immediate)\n", op);
	  return 1;
	case 1: // ---001-- $86 STX - Store X Register : ZeroPage
	  PCReg++; // op consumes the param
	  LOG_INSTRUCTION("STX $%02x", opParam);
          TEST_LOG_SET_INFO("$%02X = %02X", opParam, CpuReadByteForLog(opParam));
          TEST_LOG_1_ARG("STX");
	  CpuWriteByte(opParam, XReg);
	  break;
        case 2: // ---010-- Other ops
          switch(op & 0x3) {
          case 0: printf("line %u: op $%02X not implemented\n", __LINE__, op); return 1;
          case 1: printf("line %u: op $%02X not implemented\n", __LINE__, op); return 1;
          case 2: // $8A TXA - Trasmit X to Accumulator
            TEST_LOG_SET_INFO("");
            TEST_LOG_0_ARGS("TXA");
            AReg = XReg;
            UPDATE_STATUS_ZERO_NEGATIVE(AReg);
            break;
          case 3: printf("line %u: op $%02X not implemented\n", __LINE__, op); return 1;
          }
          break;
	case 3: // ---011-- $8E STX - Store X Register : Absolute
	  {
            opParam2 = CpuReadByte(PCReg+1);
	    ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
	    PCReg += 2; // op consumes 2 bytes
            TEST_LOG_SET_INFO("$%04X = %02X", immediate, CpuReadByteForLog(immediate));
            TEST_LOG_2_ARGS("STX");
	    LOG_INSTRUCTION("STX $%04x", immediate);
	    CpuWriteByte(immediate, XReg);
	  }
	  break;
	case 4: // ---100-- ???
          printf("in STX table, op $%02X not implemented\n", op); return 1;
	  break;
	case 5: // ---101-- $96 STX - Store X Register : ZeroPage,Y
	  PCReg++; // consumes opParam
	  {
	    ubyte addr = opParam + YReg;
	    TEST_LOG_SET_INFO("$%02X,Y @ %02X = %02X", opParam, addr, CpuReadByteForLog(addr));
	    TEST_LOG_1_ARG("STX");
	    CpuWriteByte(addr, XReg);
	    cpuCycled();
	  }
	  break;
	case 6: // ---110--
          switch(op & 0x03) {
          case 0: printf("line %u: op $%02X not implemented\n", __LINE__, op); return 1;
          case 1: printf("line %u: op $%02X not implemented\n", __LINE__, op); return 1;
          case 2: // $9A TXS - Transfer X to Stack Pointer
            TEST_LOG_SET_INFO("");
            TEST_LOG_0_ARGS("TXS");
            SPReg = XReg;
            break;
          case 3: printf("line %u: op $%02X not implemented\n", __LINE__, op); return 1;
          }
	  break;
	case 7: // ---111-- STX - Store X Register : Absolute,X
          printf("STX Absolute,X not implemented\n"); return 1;
	  break;
	}
      } else {
        // Handle accumulator operations special
        // ---010--
        if((op & 0x1C) == 0x08) {
          switch(op >> 5) {
          case 0: // $0A ASL - Arithmetic Shift Left (Accumulator)
	    TEST_LOG_SET_INFO("A");
            TEST_LOG_0_ARGS("ASL");
	    {
	      ubyte result = AReg << 1;
	      ubyte newFlags = result & STATUS_FLAG_NEGATIVE;
	      if(AReg & 0x80) {
		newFlags |= STATUS_FLAG_CARRY;
	      }
	      if(result == 0) {
		newFlags |= STATUS_FLAG_ZERO;
	      }
	      {
		ubyte oldPReg = PReg;
		PReg = newFlags | (PReg & ~STATUS_MASK_CZN);
		LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, M, PSTATUS_DIFF_ARGS(oldPReg));
	      }
	      AReg = result;
	    }
            break;
          case 1: // $2A ROL - Rotate Left (Accumulator)
	    TEST_LOG_SET_INFO("A");
            TEST_LOG_0_ARGS("ROL");
	    {
	      ubyte result = AReg << 1 | (PReg&STATUS_FLAG_CARRY);
	      ubyte newFlags = result & STATUS_FLAG_NEGATIVE;
	      if(AReg & 0x80) {
		newFlags |= STATUS_FLAG_CARRY;
	      }
	      if(result == 0) {
		newFlags |= STATUS_FLAG_ZERO;
	      }
	      {
		ubyte oldPReg = PReg;
		PReg = newFlags | (PReg & ~STATUS_MASK_CZN);
		LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, M, PSTATUS_DIFF_ARGS(oldPReg));
	      }
	      AReg = result;
	    }
            break;
          case 2: // $4A LSR - Logical Shift Right (Accumulator)
	    TEST_LOG_SET_INFO("A");
            TEST_LOG_0_ARGS("LSR");
	    {
	      ubyte result = AReg >> 1;
	      ubyte newFlags = AReg & STATUS_FLAG_CARRY;
	      if(result == 0) {
		newFlags |= STATUS_FLAG_ZERO;
	      }
	      {
		ubyte oldPReg = PReg;
		PReg = newFlags | (PReg & ~STATUS_MASK_CZN);
		LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, M, PSTATUS_DIFF_ARGS(oldPReg));
	      }
	      AReg = result;
	    }
            break;
          case 3: // $6A ROR - Rotate Right (Accumulator)
	    TEST_LOG_SET_INFO("A");
            TEST_LOG_0_ARGS("ROR");
	    {
	      ubyte result = (PReg << 7) | (AReg >> 1);
	      ubyte newFlags = (PReg << 7) | AReg & STATUS_FLAG_CARRY;
	      if(result == 0) {
		newFlags |= STATUS_FLAG_ZERO;
	      }
	      {
		ubyte oldPReg = PReg;
		PReg = newFlags | (PReg & ~STATUS_MASK_CZN);
		LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, M, PSTATUS_DIFF_ARGS(oldPReg));
	      }
	      AReg = result;
	    }
            break;
          case 4: // $8A TXA - Transmit X to Accumulator
            // Should be impossible, should be handled in the
            // previous block
            ASSERT_INVALID_CODE_PATH(__LINE__);
          case 5: // $AA TAX - Transfer Accumulator to X
            TEST_LOG_SET_INFO("");
            TEST_LOG_0_ARGS("TAX");
            XReg = AReg;
            UPDATE_STATUS_ZERO_NEGATIVE(XReg);
            break;
          case 6: // $CA
            LOG_INSTRUCTION("DEX X = $%02X", XReg);
            TEST_LOG_SET_INFO("");
            TEST_LOG_0_ARGS("DEX");
            XReg--;
            UPDATE_STATUS_ZERO_NEGATIVE(XReg);
            break;
          case 7: // $EA NOP - No Operation
            TEST_LOG_SET_INFO("");
            TEST_LOG_0_ARGS("NOP");
            break;
          }
        } else {

	  ubyte M;
	  int MWriteBackAddr;
          switch((op >> 2) & 0x07) {
          case 0: // ---000-- Immediate
            PCReg++; // consumes opParam
            TEST_LOG_SET_1_ARG();
	    MWriteBackAddr = -1;
            M = opParam;
            LOG_INSTRUCTION("#immediate is $%02X", M);
            TEST_LOG_SET_INFO("#$%02X", M);
            break;
          case 1: // ---001-- ZeroPage
            PCReg++; // consumes opParam
            TEST_LOG_SET_1_ARG();
	    MWriteBackAddr = opParam;
            M = CpuReadByte(opParam);
            TEST_LOG_SET_INFO("$%02X = %02X", opParam, M);
            LOG_INSTRUCTION("ZeroPage $%02X is %u", opParam, M);
            break;
          case 2: // ---010-- Accumulator
	    ASSERT_INVALID_CODE_PATH(__LINE__);
            break;
          case 3: // ---011-- Absolute
            opParam2 = CpuReadByte(PCReg+1);
            {
              ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
              PCReg += 2;
              TEST_LOG_SET_2_ARGS();
	      MWriteBackAddr = immediate;
              M = CpuReadByte(immediate);
              TEST_LOG_SET_INFO("$%04X = %02X", immediate, M);
              LOG_INSTRUCTION("absolute $%04X is %u", immediate, M);
              //cpuCycled(); // needs another cycle for some reason
            }
            break;
          case 4: // ---100-- ????
            printf("op $%02X (cc is 10) (bbb is 100) doesn't have any code to handle it\n", op); return 1;
            break;
          case 5: // 101 ZeroPage,X | ZeroPage,Y
            PCReg++; // consumes opParam
            TEST_LOG_SET_1_ARG();
	    {
	      ubyte regValue;
	      char regChar;
	      if(op == 0xB6) {
		regValue = YReg;
		regChar = 'Y';
	      } else {
		regValue = XReg;
		regChar = 'X';
	      }
	      MWriteBackAddr = (ubyte)(regValue + opParam); 
	      M = CpuReadByte((ushort)MWriteBackAddr);
	      TEST_LOG_SET_INFO("$%02X,%c @ %02X = %02X", opParam, regChar, MWriteBackAddr, M);
	      LOG_INSTRUCTION("ZeroPage,%c  X($%02X)+$%02X is %u", regChar, regValue, opParam, M);
	      cpuCycled(); // needs another cycle for some reason
	    }
            break;
          case 6: // ---110--
	    if(op == 0xBA) { // TSX - Transfer Stack Pointer to X
              TEST_LOG_SET_INFO("");
              TEST_LOG_0_ARGS("TSX");
              XReg = SPReg;
              UPDATE_STATUS_ZERO_NEGATIVE(XReg);
	    } else { // $1A, $3A, $5A, $7A, $DA, $FA NOP (Instruction not supported)
	      TEST_LOG_SET_INFO("");
	      TEST_LOG_0_ARGS("*NOP");
	    }
            goto C10_DONE;
          case 7: // ---111-- Absolute,X | Absolute,Y
            opParam2 = CpuReadByte(PCReg+1);
            {
              ushort immediate = (((ushort)opParam2) << 8) | (ushort)opParam;
              PCReg += 2;
              TEST_LOG_SET_2_ARGS();
	      ubyte regValue;
	      char regChar;
	      if(op == 0xBE) {
		regValue = YReg;
		regChar = 'Y';
	      } else {
		regValue = XReg;
		regChar = 'X';
	      }
	      
	      MWriteBackAddr = (ushort)regValue + immediate;
              M = CpuReadByte((ushort)MWriteBackAddr);
	      TEST_LOG_SET_INFO("$%04X,%c @ %04X = %02X", immediate, regChar, MWriteBackAddr, M);
              //LOG_INSTRUCTION("absolute,X(%u) is 0x%04x (%u)", XReg, M, M);
	      if(regChar == 'X' || ((ushort)opParam+(ushort)regValue) & 0x100) {
		cpuCycled();
	      }
              break;
            }
          }

          switch(op >> 5) {
          case 0: // 000----- ASL - Arithmetic Shift Left
            TEST_LOG_PRESET_ARGS("ASL");
	    {
	      ubyte result = M << 1;
	      ubyte newFlags = result & STATUS_FLAG_NEGATIVE;
	      if(M & 0x80) {
		newFlags |= STATUS_FLAG_CARRY;
	      }
	      if(result == 0) {
		newFlags |= STATUS_FLAG_ZERO;
	      }
	      {
		ubyte oldPReg = PReg;
		PReg = newFlags | (PReg & ~STATUS_MASK_CZN);
		LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, M, PSTATUS_DIFF_ARGS(oldPReg));
	      }
	      CpuWriteByteWithIntAddr(MWriteBackAddr, result);
	      cpuCycled();
	    }
	    break;
          case 1: // 001----- ROL - Rotate Left
            TEST_LOG_PRESET_ARGS("ROL");
	    {
	      ubyte result = M << 1 | (PReg&STATUS_FLAG_CARRY);
	      ubyte newFlags = result & STATUS_FLAG_NEGATIVE;
	      if(M & 0x80) {
		newFlags |= STATUS_FLAG_CARRY;
	      }
	      if(result == 0) {
		newFlags |= STATUS_FLAG_ZERO;
	      }
	      {
		ubyte oldPReg = PReg;
		PReg = newFlags | (PReg & ~STATUS_MASK_CZN);
		LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, M, PSTATUS_DIFF_ARGS(oldPReg));
	      }
	      CpuWriteByteWithIntAddr(MWriteBackAddr, result);
	      cpuCycled();
	    }
            break;
          case 2: // 010----- LSR - Logical Shift Right
            TEST_LOG_PRESET_ARGS("LSR");
	    {
	      ubyte result = M >> 1;
	      ubyte newFlags = M & STATUS_FLAG_CARRY;
	      if(result == 0) {
		newFlags |= STATUS_FLAG_ZERO;
	      }
	      {
		ubyte oldPReg = PReg;
		PReg = newFlags | (PReg & ~STATUS_MASK_CZN);
		LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, M, PSTATUS_DIFF_ARGS(oldPReg));
	      }
	      CpuWriteByteWithIntAddr(MWriteBackAddr, result);
	      cpuCycled();
	    }
            break;
          case 3: // 011----- ROR - Rotate Right
            TEST_LOG_PRESET_ARGS("ROR");
	    {
	      ubyte result = (PReg << 7) | (M >> 1);
	      ubyte newFlags = (PReg << 7) | M & STATUS_FLAG_CARRY;
	      if(result == 0) {
		newFlags |= STATUS_FLAG_ZERO;
	      }
	      {
		ubyte oldPReg = PReg;
		PReg = newFlags | (PReg & ~STATUS_MASK_CZN);
		LOG_INSTRUCTION("Status " PSTATUS_DIFF_FMT, M, PSTATUS_DIFF_ARGS(oldPReg));
	      }
	      CpuWriteByteWithIntAddr(MWriteBackAddr, result);
	      cpuCycled();
	    }
            break;
          case 4: // 100----- STX
            ASSERT_INVALID_CODE_PATH(__LINE__);
            break;
          case 5: // 101----- LDX - Load X Register
            // todo: no instruction is address mode is accumulator
            TEST_LOG_PRESET_ARGS("LDX");
            XReg = M;
            LOG_INSTRUCTION("LDX X = %u", XReg);
            UPDATE_STATUS_ZERO_NEGATIVE(XReg);
            break;
          case 6: // 110----- DEC - Decrement Memory
            TEST_LOG_PRESET_ARGS("DEC");
	    {
	      M--;
	      UPDATE_STATUS_ZERO_NEGATIVE(M);
	      CpuWriteByteWithIntAddr(MWriteBackAddr, M);
	      cpuCycled();
	    }
            break;
          case 7: // 111----- INC - Increment Memory
            TEST_LOG_PRESET_ARGS("INC");
	    {
	      M++;
	      UPDATE_STATUS_ZERO_NEGATIVE(M);
	      CpuWriteByteWithIntAddr(MWriteBackAddr, M);
	      cpuCycled();
	    }
            break;
          }
        }
      }
    C10_DONE:
      break;
      //
      // op = ------11
    case 3:
      printf("(c=11) op=0x%02x not implemented\n", op);return 1;
      break;
    }
  }

  printf("--------------------------------------------------------------------------------\n");
  printf("REACHED_MAX_INSTRUCTIONS!\n");
  return 0;
}
