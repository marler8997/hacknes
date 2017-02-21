
An LLVM compiler
--------------------------------------------------------------------------------
For best performance, I'd like to create a compiler that takes an NES ROM, and
compiles it to LLVM byte code.

Optimal Control Latency
--------------------------------------------------------------------------------
Have the emulator detect when the hardware reads the controller memory.
When that happens, force a sleep as long as possible so that the next
frame gets rendered just before the screen flip.

