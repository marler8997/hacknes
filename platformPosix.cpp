#if 0
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "common.h"
#include "platform.h"

struct ScopedFile
{
  FILE *ptr;
  ScopedFile(FILE *ptr) : ptr(ptr)
  {
  }
  ~ScopedFile()
  {
    if(ptr) {
      fclose(ptr);
    }
  }
};



// Returns: NULL on error, prints and sets error
Buffer readFile(const char* filename)
{
  ScopedFile file(fopen(filename, "rb"));
  if(!file.ptr) {
    printf("readFile: Error: failed to open \"%s\" (e=%d)\n", filename, errno);
    return Buffer();
  }

  fseek(file.ptr, 0, SEEK_END);
  long size = ftell(file.ptr);
  // TODO: should I check if (size_t)size is same as size?

  ubyte* buffer = (ubyte*)malloc(size);
  if(!buffer) {
    printf("readFile: Error: malloc(%ld) failed (e=%d)\n", size, errno);
    return Buffer();
  }

  fseek(file.ptr, 0, SEEK_SET);

  size_t bytesRead = fread(buffer, 1, size, file.ptr);
  if(bytesRead != size) {
    printf("readFile: Error: expected fread to return %ld, but returned %u (e=%d)\n", size, bytesRead, errno);
    free(buffer);
    return Buffer();
  } else {
    return Buffer(buffer, size);
  }
}
#endif
