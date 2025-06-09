

Current development procedure
--------------------------------------------------------------------------------
Download FCEUX and enable trace logging to a file, then run hackness and you
can compre the two logs to see where the agree.

I'm using nestest.nes to develop the cpu emulator.

Uncomment ENABLE_TEST_LOG in cpu.cpp
To get the output run:
```
hacknes --start-address c000 nestest.nex > output-file
```


New Test Method
--------------------------------------------------------------------------------
I'm checking out the test roms from this site:
```
https://github.com/christopherpow/nes-test-roms
```
