
### General Info
Resolution is 256 x 240.
Every tile is 8 x 8.
That means the screen is 32 tiles by 30 tiles (960 in total).

### Tile Encoding
Every tile is encoded with 16 bytes.
The tile is an 8x8 grid of pixels.
Each pixel is represented by 2 bits.
The pixels are encoded a bit weird, the upper bit is
in the first 8 bytes of the tile, and the lower bit
is in the second 8 bytes.

00: 0- 0- 0- 1- 1- 0- 0- 0-
01: 0- 0- 0- 1- 1- 0- 0- 0-
02: 0- 0- 0- 1- 1- 0- 0- 0-
03: 0- 1- 1- 1- 1- 1- 1- 0-
04: 0- 1- 1- 1- 1- 1- 1- 0-
05: 0- 0- 0- 1- 1- 0- 0- 0-
06: 0- 0- 0- 1- 1- 0- 0- 0-
07: 0- 0- 0- 1- 1- 0- 0- 0-

08: -0 -0 -0 -1 -1 -0 -0 -0
09: -0 -0 -0 -1 -1 -0 -0 -0
0A: -0 -0 -0 -1 -1 -0 -0 -0
0B: -0 -1 -1 -1 -1 -1 -1 -0
0C: -0 -1 -1 -1 -1 -1 -1 -0
0D: -0 -0 -0 -1 -1 -0 -0 -0
0E: -0 -0 -0 -1 -1 -0 -0 -0
0F: -0 -0 -0 -1 -1 -0 -0 -0

### The NameTable
The "nametable" is 1024 bytes. The first 960 bytes represent
the screen tiles where each value is an index into the pattern table.
The last 64 bytes are used to add extra color information to each
pattern.


### Color Palette
The NES has 52 colors (even though there is room for 64).
However, only 16 can be used at a time.
There are 2 palettes (each with 16 entries) ($3F00-$3F0F and $3F10-$3F1F).
