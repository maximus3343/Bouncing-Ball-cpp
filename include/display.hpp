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

void adjust_window_size(GLFWwindow *window, int width, int height);
