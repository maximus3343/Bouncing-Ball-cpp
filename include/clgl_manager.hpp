#pragma once
#define CL_HPP_ENABLE_EXCEPTIONS
#pragma OPENCL EXTENSION cl_intel_printf : enable
#include "../include/ball.hpp"
#include "../include/display.hpp"
#include "../include/kernel.hpp"
#include <CL/opencl.hpp>
#include <GL/glew.h>
#include <GL/glx.h>
#include <GLFW/glfw3.h>
#include <X11/Xlib.h>
#include <iostream>
#include <string>
#include <vector>

// Handles OpenCL and OpenGL interoperability.
class CLGL_Manager {
public:
  CLGL_Manager(int num_balls);
  ~CLGL_Manager();

  // Must be called first.
  // Returns window to be used by OpenGL.
  GLFWwindow *init(int width, int height);

  void update_balls();

  void draw_balls();

private:
  cl::Platform _platform; // Only one platform needed.
  cl::Device _gpu_device;
  cl::Device _cpu_device;
  cl::Context _context;
  cl::CommandQueue _queue;
  cl::Program _program;

  const int _num_balls;
  const int _num_vertices{200}; // Num of vertices to display each ball.
  cl::Buffer _balls_buffer;
  cl::BufferGL _vertices_buffer;          // Use with OpenCL.
  GLuint _vertices_buffer_id{0}, _vao{0}; // VBO and VAO.

  // Init OpenGL.
  bool init_GLFW();
  bool init_GLEW();

  // Responsible to initialize the OpenCL environment.
  void init_opencl();

  // Creates and compile the kernel as a program.
  void init_program(const std::string &kernel_source);

  GLFWwindow *create_window(int width, int height, const std::string &title);

  // Create the vert_buffer shared b/w OpenGL and OpenCL.
  void create_vert_buffer();

  // Given the current pos_buffer, update the vertices.
  void update_vertices();

  // Updates coords based on speed.
  void update_pos();

  // Handles collisions with the walls.
  void handle_wall_colls();

  // Handles collisions with balls.
  void handle_ball_colls();
};

// Compiles shader and returns it.
// Type of shader to compile, and its associated source code.
GLuint compile_shader(GLenum type, const char *source);

// Creates CLGL_Manager out of shader source codes.
GLuint create_shader_CLGL_Manager(const std::string &vertexShaderSource,
                                  const std::string &fragmentShaderSource);

// Tries to compile the kernel, and outputs error if .cl code is wrong.
// Used this since I did not have a compiler for the kernel code.
cl::Kernel try_kernel(cl::Program &prog, const std::string &fn_name);