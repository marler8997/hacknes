#pragma once

// Returns: false on error, logs errors
bool setupGraphics();
void cleanupGraphics();

// Returns a buffer that holds and array of pixels.
// Each pixel is an index into the NES palette.
ubyte* getPixelBuffer();

