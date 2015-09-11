#include <stdint.h>
#include <stdio.h>

#include "renderer_platform.h"
#include "renderer_math.h"

struct GameOffscreenBuffer {
  void *memory;
  int width;
  int height;
  int bytes_per_pixel;
  int max_width;  // We'll only allocate this much
  int max_height;
  int *z_buffer;
  bool32 is_initialized;
};

struct FileReadResult {
  void *memory;
  u64 memory_size;
};

struct Face {
  // Three vertices
  int v[3];

  // Texture coordinates
  int uvs[3];
};

struct Model {
  bool32 is_loaded;

  v3 *vertices;
  int vert_count;

  Face *faces;
  int face_count;

  v2i *texture_coords;
  int tc_count;
};