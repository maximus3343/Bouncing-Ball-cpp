#pragma once
#include "ball.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <iostream>

// Shader source code.
extern const char *vertexShaderSource;
extern const char *fragmentShaderSource;

// Compiles shader and returns it.
// Type of shader to compile, and its associated source code.
GLuint compile_shader(GLenum type, const char *source);

// Creates program out of shader source codes.
GLuint create_shader_program(const std::string &vertexShaderSource,
                             const std::string &fragmentShaderSource);

/*
    The display methods accept coordinates and sizes within a Cartesian space
   ranging from -1.0 to 1.0, which means that any resizing to the appropriate
   pixel space is handled here.
*/

// Displays point given a viewport Width X Width.
template <int width>
void display_point(const float x, const float y, const float size,
                   const std::array<float, 3> &colors) {
  // Sets point size.
  // Only need to multiply it by the width to keep appropriate scale.
  glPointSize(size * width);

  // Set color of the point.
  glColor3f(colors[0], colors[1], colors[2]);

  // Begin drawing.
  glBegin(GL_POINTS);
  glVertex2f(x, y);
  glEnd();
}