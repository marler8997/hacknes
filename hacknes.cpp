#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "platform.h"
#include "cartridge.h"
#include "cpu.h"

int main(int argc, char* argv)
{
  //if(!cartridge.load("D:\\nes\\smb.nes")) {
  //return 1;
  //}
  if(!cartridge.load("C:\\nes\\nestest.nes")) {
    return 1;
  }
  return runCpu();
}
