#include "../include/display.hpp"
#include <GLFW/glfw3.h>
#include <cstdlib>

// Vertex shader source code
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos; // Takes (x,y) as input.
    layout (location = 1) in vec3 color;
    void main() {
        gl_Position = vec4(aPos, 1.0, 1.0);
        frag_color = color; // Pass vertex color to fragment shader.
    }
)";

// Fragment shader source code
const char *fragmentShaderSource = R"(
    #version 330 core
    in vec3 frag_color;
    out vec4 final_color;
    void main() {
        final_color = vec4(frag_color, 1.0);  // Set the triangle color to green
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