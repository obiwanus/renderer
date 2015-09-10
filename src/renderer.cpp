#ifndef RENDERER_CPP
#define RENDERER_CPP

#include <stdio.h>
#include <limits.h>

global Model g_model;

inline void SetPixel(int x, int y, u32 color) {
  // Point 0, 0 is in the left bottom corner
  if (x < 0 || y < 0 || x >= g_game_backbuffer.width ||
      y >= g_game_backbuffer.height)
    return;

  int pitch = g_game_backbuffer.width * g_game_backbuffer.bytes_per_pixel;
  u8 *row = (u8 *)g_game_backbuffer.memory +
            (g_game_backbuffer.height - 1) * pitch - pitch * y +
            x * g_game_backbuffer.bytes_per_pixel;
  u32 *Pixel = (u32 *)row;
  *Pixel = color;
}

internal void DebugLine(int x0, int y0, int x1, int y1, u32 color) {
  bool32 steep = false;
  if (Abs(x1 - x0) < Abs(y1 - y0)) {
    steep = true;
    swap_int(&x0, &y0);
    swap_int(&x1, &y1);
  }
  if (x0 > x1) {
    swap_int(&x0, &x1);
    swap_int(&y0, &y1);
  }

  int dx = x1 - x0;
  int dy = y1 - y0;
  int derror2 = Abs(dy) * 2;
  int error2 = 0;
  int y = y0;

  for (int x = x0; x <= x1; x++) {
    if (steep)
      SetPixel(y, x, color);
    else
      SetPixel(x, y, color);

    error2 += derror2;

    if (error2 > 1) {
      y += (y1 > y0 ? 1 : -1);
      error2 -= 2 * dx;
    }
  }
}

internal void Triangle(v3i *p, u32 color, int *z_buffer) {
  v3i *p0 = &p[0];
  v3i *p1 = &p[1];
  v3i *p2 = &p[2];

  // Sort points by y
  if (p0->y > p1->y) swap_pointers(&p0, &p1);
  if (p1->y > p2->y) swap_pointers(&p1, &p2);
  if (p0->y > p1->y) swap_pointers(&p0, &p1);

  v3i long_side = *p2 - *p0;
  int total_height = long_side.y;
  int segment_height = 0;

  for (int y = p0->y; y <= p2->y; ++y) {
    bool32 top_half = (y > p1->y) || (p0->y == p1->y);
    v3i short_side = top_half ? (*p2 - *p1) : (*p1 - *p0);
    int y0 = (top_half ? p1->y : p0->y);
    r32 segment_share = static_cast<r32>(y - y0) / short_side.y;
    r32 total_share = static_cast<r32>(y - p0->y) / total_height;

    v3i a = short_side * segment_share + (top_half ? *p1 : *p0);
    v3i b = long_side * total_share + *p0;

    // Draw horizontal line
    {
      if (a.x > b.x) swap_pointers(&a, &b);

      int x0 = a.x;
      int x1 = b.x;
      if (x0 < 0) x0 = 0;
      if (x1 >= g_game_backbuffer.width) x1 = g_game_backbuffer.width - 1;
      if (y < 0 || y >= g_game_backbuffer.height) continue;

      int pitch = g_game_backbuffer.width * g_game_backbuffer.bytes_per_pixel;
      u8 *row = (u8 *)g_game_backbuffer.memory +
                (g_game_backbuffer.height - 1) * pitch - pitch * y +
                x0 * g_game_backbuffer.bytes_per_pixel;
      u32 *Pixel = (u32 *)row;

      for (int x = x0; x <= x1; ++x) {
        r32 share =
            (b.x == a.x) ? 1.0f : static_cast<r32>(x - a.x) / (b.x - a.x);
        v3i result = a + share * (b - a);
        int index = result.y * g_game_backbuffer.width + result.x;
        if (z_buffer[index] < result.z) {
          z_buffer[index] = result.z;
          *Pixel = color;
        }
        Pixel++;
      }
    }
  }
}

internal void DebugTriangle(v2i *p0, v2i *p1, v2i *p2, u32 color) {
  DebugLine(p0->x, p0->y, p1->x, p1->y, color);
  DebugLine(p0->x, p0->y, p2->x, p2->y, color);
  DebugLine(p1->x, p1->y, p2->x, p2->y, color);
}

internal void LoadModelFromFile(char *filename) {
  FILE *file;
  fopen_s(&file, filename, "rb");

  const int kMaxChars = 200;
  char buffer[kMaxChars];
  char line_type[3];

  // Count vertices and faces
  while (fgets(buffer, kMaxChars, file)) {
    sscanf_s(buffer, "%s ", line_type, 3);
    if (strcmp(line_type, "v") == 0) {
      g_model.vert_count++;
    } else if (strcmp(line_type, "f") == 0) {
      g_model.face_count++;
    }
  }

  // Allocate space for data
  g_model.vertices = static_cast<v3 *>(VirtualAlloc(
      0, sizeof(v3) * g_model.vert_count, MEM_COMMIT, PAGE_READWRITE));
  g_model.faces = static_cast<Face *>(VirtualAlloc(
      0, sizeof(Face) * g_model.face_count, MEM_COMMIT, PAGE_READWRITE));
  v3 *v = g_model.vertices;
  Face *f = g_model.faces;

  // Fill model data
  fseek(file, 0, SEEK_SET);
  while (fgets(buffer, kMaxChars, file)) {
    sscanf_s(buffer, "%s ", line_type, 3);
    if (strcmp(line_type, "v") == 0) {
      sscanf_s(buffer, "v %f %f %f", &v->x, &v->y, &v->z);
      v++;
    } else if (strcmp(line_type, "f") == 0) {
      char v1[30], v2[30], v3[30];  // vertex data
      sscanf_s(buffer, "f %s %s %s", v1, 30, v2, 30, v3, 30);
      sscanf_s(v1, "%d/%d", &f->v[0], &f->uvs[0]);
      sscanf_s(v2, "%d/%d", &f->v[1], &f->uvs[1]);
      sscanf_s(v3, "%d/%d", &f->v[2], &f->uvs[2]);
      f++;
    }
  }

  fclose(file);
  g_model.is_loaded = true;
}

inline u32 GetGrayColor(r32 intensity) {
  u32 Grey = static_cast<u32>(0xFF * intensity);
  u32 result = Grey << 16 | Grey << 8 | Grey;
  return result;
}

internal void Render() {
  v3 light_direction = {0, 0, -1.0f};
  light_direction = Normalize(light_direction);
  v3i p[3];
  int height = g_game_backbuffer.height;
  int width = g_game_backbuffer.width;
  if (!g_model.is_loaded) LoadModelFromFile("african_head.model");
  if (!g_game_backbuffer.is_initialized) {
    g_game_backbuffer.z_buffer = (int *)VirtualAlloc(
        0, width * height * sizeof(int), MEM_COMMIT, PAGE_READWRITE);
    for (int i = 0; i < width * height; ++i) {
      g_game_backbuffer.z_buffer[i] = INT_MIN;
    }
    g_game_backbuffer.is_initialized = true;
  }

  // Draw model
  for (int i = 0; i < g_model.face_count; ++i) {
    Face *face = &g_model.faces[i];

    v3 vert[3];
    // Look up actual coordinates from the model
    for (int j = 0; j < 3; ++j) {
      vert[j] = g_model.vertices[face->v[j] - 1];
    }

    v3 normal = Normalize(CrossProduct(vert[2] - vert[0], vert[1] - vert[0]));
    r32 intensity = DotProduct(normal, light_direction);
    if (intensity <= 0) continue;

    // Adjust the vertices to fit the window size
    for (int j = 0; j < 3; ++j) {
      for (int k = 0; k < 3; ++k) {
        p[j].e[k] = static_cast<int>((vert[j].e[k] + 1.0f) * height / 2.0f);
      }
    }

    Triangle(p, GetGrayColor(intensity), g_game_backbuffer.z_buffer);
  }

  // u32 color = 0x00AAAAAA;
  // v2i p0[3] = {{10, 70}, {50, 160}, {70, 100}};
  // v2i p1[3] = {{180, 50}, {150, 1}, {70, 180}};
  // v2i p2[3] = {{180, 150}, {120, 160}, {130, 180}};

  // DebugTriangle(&p0[0], &p0[1], &p0[2], 0x00FF0000);
  // DebugTriangle(&p1[0], &p1[1], &p1[2], 0x00FF0000);
  // DebugTriangle(&p2[0], &p2[1], &p2[2], 0x00FF0000);

  // Triangle(&p0[0], &p0[1], &p0[2], color);
  // Triangle(&p1[0], &p1[1], &p1[2], color);
  // Triangle(&p2[0], &p2[1], &p2[2], color);
}

#endif  // RENDERER_CPP