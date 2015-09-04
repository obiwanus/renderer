#ifndef RENDERER_CPP
#define RENDERER_CPP

#include <stdio.h>

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

struct Line {
  int x0, y0;
  int x1, y1;
  int dx;
  int dy;
  int sign_dx;
  int x;  // output
  bool32 left_side;
  bool32 steep;
  r32 error;
  r32 derror;

  Line(v2i *p0, v2i *p1, bool32 left_side);

  int GetNextX();
};

Line::Line(v2i *p0, v2i *p1, bool32 is_left_side) {
  x0 = p0->x, y0 = p0->y;
  x1 = p1->x, y1 = p1->y;

  left_side = is_left_side;
  dx = x1 - x0;
  dy = y1 - y0;  // always positive
  steep = Abs(dy) > Abs(dx);
  derror = Abs(dy) / (r32)Abs(dx);
  error = 0;
  sign_dx = (dx > 0) ? 1 : -1;
  x = x0;
}

int Line::GetNextX(void) {
  if (steep) {
    error += 1.0f;
    if (error >= derror) {
      error -= derror;
      x += sign_dx;
    }
  } else {
    while (error < 0.5f && x < x1) {
      error += derror;
      x += sign_dx;
    }
    error -= 1.0f;
  }
  return x;
}

internal void HorizontalLine(int x0, int x1, int y, u32 color) {
  if (x1 < x0) swap_int(&x0, &x1);

  if (x0 < 0) x0 = 0;
  if (x1 >= g_game_backbuffer.width) x1 = g_game_backbuffer.width - 1;
  if (y < 0 || y >= g_game_backbuffer.height) return;

  int pitch = g_game_backbuffer.width * g_game_backbuffer.bytes_per_pixel;
  u8 *row = (u8 *)g_game_backbuffer.memory +
            (g_game_backbuffer.height - 1) * pitch - pitch * y +
            x0 * g_game_backbuffer.bytes_per_pixel;
  u32 *Pixel = (u32 *)row;

  for (int x = x0; x <= x1; ++x) {
    *Pixel++ = color;
  }
}

internal void Triangle(v2i *p0, v2i *p1, v2i *p2, u32 color) {
  // Sort points by y
  if (p0->y > p1->y) swap_pointers(&p0, &p1);
  if (p1->y > p2->y) swap_pointers(&p1, &p2);
  if (p0->y > p1->y) swap_pointers(&p0, &p1);

  {
    // Line p0 -> p1
    bool32 is_left_side = (p1->x < p2->x);
    Line line1 = Line(p0, p1, is_left_side);
    Line line2 = Line(p0, p2, !is_left_side);
    Line line3 = Line(p1, p2, is_left_side);

    // Bottom half
    for (int y = p0->y; y < p1->y; ++y) {
      HorizontalLine(line1.GetNextX(), line2.GetNextX(), y, color);
    }

    // Top half
    for (int y = p1->y; y <= p2->y; ++y) {
      HorizontalLine(line2.GetNextX(), line3.GetNextX(), y, color);
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
      sscanf_s(v1, "%d", &f->v1);
      sscanf_s(v2, "%d", &f->v2);
      sscanf_s(v3, "%d", &f->v3);
      if (f->v3 == 0) {
        int a = 0;
      }
      f++;
    }
  }

  fclose(file);
  g_model.is_loaded = true;
}

internal void Render() {
  if (!g_model.is_loaded) LoadModelFromFile("african_head.model");

  u32 color = 0x00999999;

  // // Draw model
  // for (int i = 0; i < g_model.face_count; ++i) {
  //   Face *face = &g_model.faces[i];
  //   for (int j = 0; j < 3; ++j) {
  //     v3 *vert0 = &g_model.vertices[face->e[j] - 1];
  //     v3 *vert1 = &g_model.vertices[face->e[(j+1) % 3] - 1];

  //     int height = g_game_backbuffer.height;

  //     int x0 = static_cast<int>((vert0->x + 1.0f) * height / 2.0f);
  //     int y0 = static_cast<int>((vert0->y + 1.0f) * height / 2.0f);

  //     int x1 = static_cast<int>((vert1->x + 1.0f) * height / 2.0f);
  //     int y1 = static_cast<int>((vert1->y + 1.0f) * height / 2.0f);

  //     Line(x0, y0, x1, y1, color);
  //   }
  // }

  v2i p0[3] = {{10, 70}, {50, 160}, {70, 100}};
  v2i p1[3] = {{180, 50}, {150, 1}, {70, 180}};
  v2i p2[3] = {{180, 150}, {120, 160}, {130, 180}};

  DebugTriangle(&p0[0], &p0[1], &p0[2], 0x00FF0000);
  // DebugTriangle(&p1[0], &p1[1], &p1[2], 0x00FF0000);
  // DebugTriangle(&p2[0], &p2[1], &p2[2], 0x00FF0000);

  Triangle(&p0[0], &p0[1], &p0[2], color);
  // Triangle(&p1[0], &p1[1], &p1[2], color);
  // Triangle(&p2[0], &p2[1], &p2[2], color);
}

#endif  // RENDERER_CPP