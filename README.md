

Current development procedure
--------------------------------------------------------------------------------
I'm using nestest.nes to develop the cpu emulator.

Uncomment ENABLE_TEST_LOG in cpu.cpp
To get the output run:
```
hacknes --start-address c000 nestest.nex > output-file
```
