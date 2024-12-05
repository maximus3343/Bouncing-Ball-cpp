#include <chrono>
#include <iostream>
#include <thread>
#pragma once

// Not very accurate, but gives out an approximate FPS counter.
class FPS_Counter {
public:
  FPS_Counter();

  // Update frame_count and prints FPS if more than 1 second elapsed.
  void update();

private:
  // Number of frames rendered per second.
  int _frame_count;
  // Time elapsed since last FPS calculation.
  double _elapsed_time;
  // Start time for FPS calculation.
  std::chrono::high_resolution_clock::time_point _start_time;
};

class FPS_Cap {
public:
  FPS_Cap(const int target_fps);

  void limit(); // Limits the FPS cap for each iteration.

private:
  const int _target_fps;
  const std::chrono::milliseconds _frame_duration;
  std::chrono::high_resolution_clock::time_point _frame_start;
};