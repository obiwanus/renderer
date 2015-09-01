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


union Face {
  // Three vertices
  struct {
    int v1;
    int v2;
    int v3;
  };

  int e[3];
};


struct Model {
  bool32 is_loaded;

  v3 *vertices;
  int vert_count;

  Face *faces;
  int face_count;
};