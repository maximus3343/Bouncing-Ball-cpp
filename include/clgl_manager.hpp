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
  CLGL_Manager(int num_balls, int num_vertices);
  ~CLGL_Manager();

  // Must be called first.
  // Returns window to be used by OpenGL.
  GLFWwindow *init(int width, int height);

  // Update all the ball positions, handle their collisions with other balls and
  // boundaries.
  void update_balls();

  // Uses the buffer of vertices vbo_cl stored on the GPU to draw the balls with
  // OpenGL.
  void draw_balls();

private:
  cl::Platform _platform; // Only one platform needed.
  cl::Device _gpu_device;
  cl::Device _cpu_device;
  cl::Context _context;
  cl::CommandQueue _queue;
  cl::Program _program;

  const int _num_balls;
  const int _num_vertices; // Num of vertices to display each ball.
  cl::Buffer _balls_buffer;
  cl::BufferGL _vbo_cl;     // Use with OpenCL.
  GLuint _vbos[2], _vao{0}; // VBO and VAO.

  // Init OpenGL.
  bool init_GLFW();
  bool init_GLEW();

  // Responsible to initialize the OpenCL environment.
  void init_opencl();

  // Creates and compile the kernel as a program.
  void init_program(const std::string &kernel_source);

  GLFWwindow *create_window(int width, int height, const std::string &title);

  // Create the buffer of vertices shared b/w OpenGL and OpenCL.
  void create_vbo();

  // Given the current ball position, update the vertices stored in the vbo_cl.
  void update_vertices();

  // Print the vertices. For debugging only.
  void print_vertices();

  // Updates coords based on speed.
  void update_pos();

  // Handles collisions with the walls.
  // Also handles the gravity for the balls.
  void handle_wall_colls();

  // Handles collisions with balls.
  void handle_ball_colls();
};

// Tries to compile the kernel, and outputs error if .cl code is wrong.
// Used this since I did not have a compiler for the kernel code.
cl::Kernel try_kernel(cl::Program &prog, const std::string &fn_name);