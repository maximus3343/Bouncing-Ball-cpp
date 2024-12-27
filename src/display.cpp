#include "../include/display.hpp"
#include <GLFW/glfw3.h>

// Vertex shader source code
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 pos; // Takes (x,y,z) as input.
    layout (location = 1) in vec3 color;

    out vec3 frag_color;
    void main() {
        gl_Position = vec4(pos, 1.0);
        frag_color = color;
    }
)";

// Fragment shader source code
const char *fragmentShaderSource = R"(
    #version 330 core
    in vec3 frag_color;
    out vec4 final_color;
    void main() {
        final_color = vec4(frag_color, 1.0);
    }
)";

GLuint compile_shader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  // Check for compile errors
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "Shader Compilation Error: " << infoLog << std::endl;
  }
  return shader;
}

GLuint create_shader_program(const std::string &vertexShaderSource,
                             const std::string &fragmentShaderSource) {
  GLuint vertexShader =
      compile_shader(GL_VERTEX_SHADER, vertexShaderSource.data());
  GLuint fragmentShader =
      compile_shader(GL_FRAGMENT_SHADER, fragmentShaderSource.data());

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  // Check for linking errors
  GLint success;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << "Program Linking Error: " << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return shaderProgram;
}

void adjust_window_size(GLFWwindow *window, int width, int height) {
  if (height == 0)
    height = 1;
  if (width == 0)
    width = 1;

  glViewport(0, 0, width, height);
}