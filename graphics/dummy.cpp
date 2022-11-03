#include <stdlib.h>

#include "../common.h"
#include "../ppu.h"

bool setupGraphics() {
    return true;
}

void cleanupGraphics() {
}

ubyte buffer[SCREEN_PIXEL_WIDTH*SCREEN_PIXEL_HEIGHT];
ubyte* getPixelBuffer() {
    return buffer;
}
