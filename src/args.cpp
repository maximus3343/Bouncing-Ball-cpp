#include "../include/args.hpp"
#include <cstdlib>

void process_args(int argc, char **argv, int &num_balls, int &num_vertices) {

  num_balls = 0;
  num_vertices = 0;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-b" || arg == "--balls") {
      if (i + 1 < argc)
        num_balls = std::stoi(argv[++i]);
      else {
        std::cerr << "Error: --balls flag requires a value." << std::endl;
        exit(EXIT_FAILURE);
      }
    } else if (arg == "-v" || arg == "--vertices") {
      if (i + 1 < argc)
        num_vertices = std::stoi(argv[++i]);
      else {
        std::cerr << "Error: --vertices flag requires a value." << std::endl;
        exit(EXIT_FAILURE);
      }
    } else {
      std::cerr << argv[i] << ": Unknown argument." << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  // Defaults args.
  if (num_balls == 0)
    num_balls = 5;
  if (num_vertices == 0)
    num_vertices = 40;
}