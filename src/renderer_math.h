#ifndef RENDERER_MATH_H
#define RENDERER_MATH_H


#include <math.h>
#include <memory.h>


union v2 {
  struct {
    r32 x, y;
  };
  struct {
    r32 u, v;
  };
  r32 E[2];
};


inline v2 operator*(r32 scalar, v2 a) {
  v2 result;

  result.x = a.x * scalar;
  result.y = a.y * scalar;

  return result;
}


inline v2 operator*(v2 a, r32 scalar) {
  return scalar * a;
}


inline v2 & operator*=(v2 &a, r32 scalar) {
  a = a * scalar;
  return a;
}


inline v2 operator+(v2 a, v2 b) {
  v2 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;

  return result;
}


inline v2 & operator+=(v2 &a, v2 b) {
  a = a + b;
  return a;
}


inline v2 operator-(v2 a, v2 b) {
  v2 result;

  result.x = a.x - b.x;
  result.y = a.y - b.y;

  return result;
}


inline v2 & operator-=(v2 &a, v2 b)
{
  a = a - b;

  return a;
}


// Unary
inline v2 operator-(v2 a) {
  v2 result;

  result.x = -a.x;
  result.y = -a.y;

  return result;
}


inline r32 Square(r32 value) {
  return value * value;
}


inline int Square(int value) {
  return value * value;
}


inline r32 SquareRoot(r32 value) {
  r32 result = sqrtf(value);
  return result;
}


inline r32 Abs(r32 value) {
  if (value >= 0)
    return value;
  else
    return -value;
}


inline int Abs(int value) {
  if (value >= 0)
    return value;
  else
    return -value;
}


inline void swap_int(int *a, int *b) {
  int buffer = *a;
  *a = *b;
  *b = buffer;
}


inline void swap_pointers(void *a, void *b) {
  const int size = sizeof(void *);
  char temp[size];

  memcpy(temp, b, size);
  memcpy(b, a, size);
  memcpy(a, temp, size);
}


inline r32 V2Length(v2 vector) {
  r32 result = SquareRoot(Square(vector.x) + Square(vector.y));
  return result;
}


inline r32 DistanceBetween(v2 dot1, v2 dot2) {
  v2 diff = dot2 - dot1;
  return V2Length(diff);
}


inline r32 DotProduct(v2 vector1, v2 vector2) {
  r32 result = vector1.x * vector2.x + vector1.y * vector2.y;
  return result;
}


// Integer vector 2

union v2i {
  struct {
    int x, y;
  };
  int e[2];
};


// Vector 3

union v3 {
  struct {
    r32 x, y, z;
  };
  r32 E[3];
};


inline v3 operator*(r32 scalar, v3 a) {
  v3 result;

  result.x = a.x * scalar;
  result.y = a.y * scalar;
  result.z = a.z * scalar;

  return result;
}


inline v3 operator*(v3 a, r32 scalar) {
  return scalar * a;
}


inline v3 & operator*=(v3 &a, r32 scalar) {
  a = a * scalar;
  return a;
}


inline v3 operator+(v3 a, v3 b) {
  v3 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;

  return result;
}


inline v3 & operator+=(v3 &a, v3 b) {
  a = a + b;
  return a;
}


inline v3 operator-(v3 a, v3 b) {
  v3 result;

  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;

  return result;
}


inline v3 & operator-=(v3 &a, v3 b) {
  a = a - b;
  return a;
}


// Unary
inline v3 operator-(v3 a) {
  v3 result;

  result.x = -a.x;
  result.y = -a.y;
  result.z = -a.z;

  return result;
}


#endif