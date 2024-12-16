#include "../include/clgl_manager.hpp"
#include "../include/fps.hpp"

// Window size.
constexpr int width = 1600, height = 1600;
// Reach up a max of 60fps. Should always be reached, program is not heavy.
constexpr int target_fps = 60;

int main(int argc, char *argv[]) {

  // Creates the balls and stores them in the program.
  int num_balls = std::stoi(argv[1]);
  int num_vertices = std::stoi(argv[2]);

  CLGL_Manager prog(num_balls, num_vertices);
  auto window = prog.init(width, height);

  FPS_Counter fps_counter;
  FPS_Cap fps_cap(target_fps); // Limit FPS.

  std::cout << "GLFW version: " << glfwGetVersionString() << std::endl;

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT); // Clear image at each frame.

    prog.update_balls();

    prog.draw_balls();

    // Swap front and back buffers.
    glfwSwapBuffers(window);

    // Poll for and process events.
    glfwPollEvents();

    // Prints frames per second.
    fps_counter.update();

    // Limit FPS.
    fps_cap.limit();
  }
  // Clean up and exit.
  glfwDestroyWindow(window);

  return 0;
}