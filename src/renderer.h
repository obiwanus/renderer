#include <stdint.h>
#include <stdio.h>

#include "renderer_platform.h"
#include "renderer_math.h"


struct GameOffscreenBuffer {
  void *memory;
  int width;
  int height;
  int bytes_per_pixel;
  int max_width;   // We'll only allocate this much
  int max_height;
};


struct FileReadResult {
  void *memory;
  u64 memory_size;
};

