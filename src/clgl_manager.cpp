#include "../include/clgl_manager.hpp"
#include <CL/cl_platform.h>

CLGL_Manager::CLGL_Manager(int num_balls, int num_vertices)
    : _num_balls(num_balls), _num_vertices(num_vertices) {}

CLGL_Manager::~CLGL_Manager() { glfwTerminate(); }

GLFWwindow *CLGL_Manager::init(int width, int height) {
  // Initializes first OpenGL.
  init_GLFW();
  auto window = create_window(width, height, "Bouncing Ball");
  init_GLEW();

  // Initializes OpenCL.
  init_opencl();

  // Create the balls.
  std::vector<Ball> balls;
  balls.reserve(_num_balls);
  std::generate_n(std::back_inserter(balls), _num_balls, create_ball);

  // Create the buffer of balls on device.
  _balls_buffer = cl::Buffer(_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                             _num_balls * sizeof(Ball), balls.data());

  // Create the vertices buffer shared by OpenCL and OpenGL.
  create_vbo();

  init_program(kernel_source());

  // Create shader program to display circles.
  GLuint program =
      create_shader_program(vertexShaderSource, fragmentShaderSource);

  glUseProgram(program);

  return window;
}

bool CLGL_Manager::init_GLFW() {
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }
  // Set GLFW version and OpenGL profile.
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  return 1;
}

bool CLGL_Manager::init_GLEW() {
  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(err));
    return -1;
  }
  return 1;
}

GLFWwindow *CLGL_Manager::create_window(int width, int height,
                                        const std::string &title) {
  auto window = glfwCreateWindow(width, height, title.data(), NULL, NULL);

  if (!window) {
    fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(window);
  return window;
}

void CLGL_Manager::init_opencl() {

  // Get Platform.
  std::array<cl::Platform, 1> platforms;
  if (platforms.empty()) {
    std::cerr << "No OpenCL platforms found." << std::endl;
    return;
  }
  _platform = platforms[0];

  // Get GPU device.
  std::vector<cl::Device> devices;
  size_t max_work_group_size;
  _platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
  if (devices.empty()) {
    std::cerr << "No OpenCL GPU devices found." << std::endl;
    return;
  }
  _gpu_device = devices[0];
  _gpu_device.getInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE, &max_work_group_size);
  std::cout << "The GPU device has a maximum work group size of: "
            << max_work_group_size << std::endl;

  // Needed to get Interoperability b/w OpenGL and OpenCL.
  cl_context_properties properties[] = {
      CL_GL_CONTEXT_KHR,
      (cl_context_properties)glXGetCurrentContext(), // GLX context handle
      CL_GLX_DISPLAY_KHR,
      (cl_context_properties)XOpenDisplay(nullptr), // X11 display handle
      0};

  // Create context.
  _context = cl::Context(_gpu_device, properties);

  // Create Command Queue.
  _queue = cl::CommandQueue(_context, _gpu_device);
}

void CLGL_Manager::init_program(const std::string &kernel_source) {
  _program = cl::Program(_context, kernel_source);
  try {
    _program.build();
  } catch (const cl::BuildError &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "The program build has failed." << std::endl;
    std::string buildLog;
    _program.getBuildInfo(_gpu_device, CL_PROGRAM_BUILD_LOG, &buildLog);
    std::cerr << "Build log:\n" << buildLog << std::endl;
  }
}

void CLGL_Manager::create_vbo() {
  // Create and bind VAO.
  glGenVertexArrays(1, &_vao);
  glBindVertexArray(_vao); // Stores binded VBO and vertex attrib ptr.

  // Generate a buffer object.
  glGenBuffers(1, &_vbos[0]);
  glBindBuffer(GL_ARRAY_BUFFER,
               _vbos[0]); // Bind the buffer to the GL_ARRAY_BUFFER target.

  // Allocates memory for each vertex of each ball.
  // Each vertex has 6 components, the RGB colors and (x,y,z).
  glBufferData(GL_ARRAY_BUFFER, sizeof(cl_float3) * _num_vertices * _num_balls,
               nullptr, GL_DYNAMIC_DRAW);

  // Layout specified.
  // Three components (x, y, z) per attribute.
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cl_float3),
                        (GLvoid *)0);
  glEnableVertexAttribArray(0); // Enables position attribute to be rendered.

  // Get colors stored in GPU.
  std::vector<Ball> host_balls(_num_balls);
  // host_balls.reserve(_num_balls);
  _queue.enqueueReadBuffer(_balls_buffer, CL_TRUE, 0, _num_balls * sizeof(Ball),
                           host_balls.data());

  // Get a one-dimension array of colors from all the balls.
  // Colors will be structured as such: [R0, G0, B0, R1, G1, B1, ...].
  auto get_colors = [&host_balls, this]() {
    std::vector<float> colors;                      // Array of colors.
    colors.reserve(3 * _num_balls * _num_vertices); // RGB for each vertex.
    for (const Ball &ball : host_balls) {
      const auto ball_colors = ball.colors;
      // One single color for all vertices of a Ball.
      for (int j = 0; j < _num_vertices; j++) {
        colors.emplace_back(ball_colors[0]);
        colors.emplace_back(ball_colors[1]);
        colors.emplace_back(ball_colors[2]);
      }
    }
    return colors;
  };

  // Generate and bind the color VBO.
  glGenBuffers(1, &_vbos[1]);
  glBindBuffer(GL_ARRAY_BUFFER, _vbos[1]);
  glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float) * _num_balls * _num_vertices,
               get_colors().data(), GL_STATIC_DRAW);
  // Setting up the RGB.
  // Stored in an different VBO, not interleaved with positions.
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                        (GLvoid *)(0));
  glEnableVertexAttribArray(1);

  // Unbind the VAO to avoid accidental modifications.
  glBindVertexArray(0);
  // Unbind the VBO.
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  try {
    // Creates and assign our vertex buffer.
    // Size is defined by the underlying OpenGL buffer.
    _vbo_cl = cl::BufferGL(_context, CL_MEM_READ_WRITE, _vbos[0]);
  } catch (const cl::Error &e) {
    std::cerr << "OpenCL Error: " << e.what() << " (" << e.err() << ")"
              << std::endl;
  }
}

void CLGL_Manager::update_vertices() {
  static cl::Kernel kernel = try_kernel(_program, "compute_ball_vertices");
  kernel.setArg(0, _balls_buffer);
  kernel.setArg(1, _vbo_cl);
  kernel.setArg(2, _num_balls);
  kernel.setArg(3, _num_vertices);

  //_queue.enqueueAcquireGLObjects(&_vbo_cl);
  try {
    _queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(_num_balls));
  } catch (const cl::Error &e) {
    std::cerr << "OpenCL Error: " << e.what() << " (" << e.err() << ")"
              << std::endl;
  }
}

void CLGL_Manager::print_vertices() {
  static cl::Kernel kernel = try_kernel(_program, "print_vertices");
  kernel.setArg(0, _vbo_cl);
  kernel.setArg(1, _num_balls);
  kernel.setArg(2, _num_vertices);

  // Num of work items is dependent on num_balls.
  try {
    _queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(1));
  } catch (const cl::Error &e) {
    std::cerr << "OpenCL Error: " << e.what() << " (" << e.err() << ")"
              << std::endl;
  }
}

void CLGL_Manager::update_pos() {
  // Should be only created once.
  static cl::Kernel kernel = try_kernel(_program, "update_pos");
  kernel.setArg(0, _balls_buffer); // Updates balls on GPU.
  kernel.setArg(1, _num_balls);

  // Num of work items is dependent on num_balls.
  try {
    _queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(_num_balls));
  } catch (const cl::Error &e) {
    std::cerr << "OpenCL Error: " << e.what() << " (" << e.err() << ")"
              << std::endl;
  }
}

void CLGL_Manager::handle_wall_colls() {
  static cl::Kernel kernel = try_kernel(_program, "handle_wall_colls");
  kernel.setArg(0, _balls_buffer);
  kernel.setArg(1, _num_balls);

  try {
    // 4 Work-items allocated per ball.
    _queue.enqueueNDRangeKernel(kernel, cl::NullRange,
                                cl::NDRange(_num_balls * 4), cl::NDRange(4));
  } catch (const cl::Error &e) {
    std::cerr << "OpenCL Error: " << e.what() << " (" << e.err() << ")"
              << std::endl;
  }
}

void CLGL_Manager::handle_ball_colls() {
  static cl::Kernel kernel = try_kernel(_program, "handle_ball_colls");
  kernel.setArg(0, _balls_buffer);
  kernel.setArg(1, _num_balls);
  try {
    _queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(_num_balls),
                                cl::NDRange(1));
  } catch (const cl::Error &e) {
    std::cerr << "OpenCL Error: " << e.what() << " (" << e.err() << ")"
              << std::endl;
  }
}

void CLGL_Manager::update_balls() {
  update_pos();
  handle_wall_colls();
  handle_ball_colls();
}

void CLGL_Manager::draw_balls() {
  update_vertices();
  glBindVertexArray(_vao); // Get the binded VBO and vertex attrib.
  // print_vertices();
  for (int i = 0; i < _num_balls; i++)
    glDrawArrays(GL_TRIANGLE_FAN, _num_vertices * i, _num_vertices);
  glBindVertexArray(0);
}

cl::Kernel try_kernel(cl::Program &prog, const std::string &fn_name) {
  cl::Kernel kernel;
  try {
    kernel = cl::Kernel(prog, fn_name);
  } catch (const cl::Error &e) {
    std::cerr << "OpenCL Error: " << e.what() << " (" << e.err() << ")"
              << std::endl;
    exit(1);
  }
  return kernel;
}