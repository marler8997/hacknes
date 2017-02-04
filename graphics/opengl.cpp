#include <windows.h>
#include <stdio.h>
#include <common.h>

const PIXELFORMATDESCRIPTOR format = {
  sizeof(PIXELFORMATDESCRIPTOR),
  1, // version
  PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
  PFD_TYPE_RGBA, // PixelType (TODO: change to PFD_TYPE_COLORINDEX)
  32,            // FrameBuffer color depth (TODO: make this smaller)
  0, 0, 0, 0, 0, 0,
  0,
  0,
  0,
  0, 0, 0, 0,
  24,                 //Number of bits for the depthbuffer
  8,                  //Number of bits for the stencilbuffer
  0,                  //Number of Aux buffers in the framebuffer.
  PFD_MAIN_PLANE,
  0,
  0, 0, 0
};


ubyte* getPixelBuffer()
{
  

  
  printf("OPENGL: getPixelBuffer: not implemented\n");
  return NULL;
}
