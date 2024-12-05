#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <random>

// Ball code needs to be written in C to be compatible with OpenCL.
// Stored as a struct on the GPU device.

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  float radius;
  float mass;
  float gravity;
  float colors[3];
} Ball;

// Creates a random ball.
Ball create_ball();
