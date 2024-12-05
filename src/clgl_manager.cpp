#include "../include/clgl_manager.hpp"
#include <CL/opencl.hpp>

CLGL_Manager::CLGL_Manager(int num_balls) : _num_balls(num_balls) {}

CLGL_Manager::~CLGL_Manager() { glfwTerminate(); }

GLFWwindow *CLGL_Manager::init(int width, int height) {
  // Initializes first OpenGL.
  init_GLFW();
  auto window = create_window(width, height, "CLGL_Manager");
  init_GLEW();

  // Initializes OpenCL, and create a shared buffer to hold the Balls objects.
  init_opencl();

  // Create the balls.
  std::vector<Ball> balls(_num_balls);
  std::generate(balls.begin(), balls.end(), create_ball);

  // Create the buffer of balls on device.
  _balls_buffer = cl::Buffer(_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                             _num_balls * sizeof(Ball), balls.data());

  // Create the vertices buffer shared by OpenCL and OpenGL.
  create_vert_buffer();

  init_program(kernel_source);

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
  auto window =
      glfwCreateWindow(width, height, "Simple OpenGL CLGL_Manager", NULL, NULL);

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
  std::vector<cl::Platform> platforms(1);
  if (platforms.empty()) {
    std::cerr << "No OpenCL platforms found." << std::endl;
    return;
  }
  _platform = platforms[0];

  // Get CPU device.
  for (const auto &platform : platforms) {
    std::vector<cl::Device> devices;

    platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
    if (!devices.empty()) {
      std::cout << "Found a CPU." << std::endl;
    }
  }

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

void CLGL_Manager::create_vert_buffer() {
  // Create and bind VAO.
  glGenVertexArrays(1, &_vao);
  glBindVertexArray(_vao); // Stores binded VBO and vertex attrib ptr.

  // Generate a buffer object.
  glGenBuffers(1, &_vertices_buffer_id);
  glBindBuffer(
      GL_ARRAY_BUFFER,
      _vertices_buffer_id); // Bind the buffer to the GL_ARRAY_BUFFER target.

  // Allocate memory for the buffer (e.g., num_vertices * sizeof(float2) *
  // num_balls)
  glBufferData(GL_ARRAY_BUFFER, sizeof(cl_float2) * _num_vertices * _num_balls,
               nullptr, GL_DYNAMIC_DRAW);

  // Layout specified.
  // Two comp (x, y) per attribute.
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                        (GLvoid *)0);

  glEnableVertexAttribArray(0); // Enables position attribute to be rendered.

  // Unbind the VAO to avoid accidental modifications.
  glBindVertexArray(0);
  // Unbind the VBO.
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  try {
    // Creates and assign our vertex buffer.
    // Size is defined by the underlying OpenGL buffer.
    _vertices_buffer =
        cl::BufferGL(_context, CL_MEM_READ_WRITE, _vertices_buffer_id);
  } catch (const cl::Error &e) {
    std::cerr << "OpenCL Error: " << e.what() << " (" << e.err() << ")"
              << std::endl;
  }
}

void CLGL_Manager::update_vertices() {
  static cl::Kernel kernel = try_kernel(_program, "compute_ball_vertices");
  kernel.setArg(0, _balls_buffer);
  kernel.setArg(1, _vertices_buffer);
  kernel.setArg(2, _num_balls);
  kernel.setArg(3, _num_vertices);

  //_queue.enqueueAcquireGLObjects(&_vertices_buffer);
  try {
    _queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(_num_balls));
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