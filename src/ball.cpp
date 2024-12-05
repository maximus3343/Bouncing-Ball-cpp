#include "../include/ball.hpp"

Ball create_ball() {
  static constexpr float max_coord =
      0.85f; // Do not want ball spawning on borders: Creates a bug.
  static constexpr float max_speed = 0.040f;
  static constexpr float min_speed = 0.002f;
  static constexpr float min_grav = -0.002f;
  static constexpr float color_range = 1.0f;
  static constexpr std::array<float, 3> radii = {0.025, 0.050, 0.075};

  std::random_device rd;  // Obtain a random number from hardware
  std::mt19937 gen(rd()); // Seed the generator

  // Generates random coordinate values.
  static std::uniform_real_distribution<float> coord_value(-max_coord,
                                                           max_coord);

  // Generate random velocity values except when below min_speed.
  auto velocity_value = [&gen]() -> float {
    std::uniform_real_distribution<float> speed_value(-max_speed, max_speed);
    float velocity;
    do {
      velocity = speed_value(gen);
    } while (velocity <= min_speed &&
             velocity >= -min_speed); // Not below min_speed.
    return velocity;
  };

  // Generates random gravity values.
  static std::uniform_real_distribution<float> gravity_value(min_grav, -0.001);

  // Returns a random radius.
  auto radius_value = [&gen]() -> float {
    std::uniform_int_distribution<> index_value(0, radii.size() - 1);
    return radii[index_value(gen)];
  };

  // Generate random colors.
  static std::uniform_real_distribution<float> color_value(0.0f, color_range);
  float colors[3];
  for (int i = 0; i < 3; i++) {
    colors[i] = color_value(gen);
  }

  Ball ball;

  memcpy(ball.colors, colors, sizeof(colors));
  ball.radius = radius_value();
  ball.x = coord_value(gen);
  ball.y = coord_value(gen);
  ball.vx = velocity_value();
  ball.vy = velocity_value();
  ball.radius = radius_value();
  ball.gravity = gravity_value(gen);

  return ball;
}
