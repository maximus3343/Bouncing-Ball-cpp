#include "../include/fps.hpp"
#include <chrono>

FPS_Counter::FPS_Counter() : _frame_count(0), _elapsed_time(0.0) {
  _start_time = std::chrono::high_resolution_clock::now();
}

void FPS_Counter::update() {
  _frame_count++;
  auto current_time = std::chrono::high_resolution_clock::now();
  _elapsed_time =
      std::chrono::duration<double>(current_time - _start_time).count();

  if (_elapsed_time >= 1.0) {
    std::cout << "FPS: " << _frame_count << std::endl;
    _frame_count = 0; // Resets frame_count.
    _start_time = current_time;
  }
}

FPS_Cap::FPS_Cap(const int target_fps)
    : _target_fps(target_fps), _frame_duration(1000 / target_fps) {
  _frame_start = std::chrono::high_resolution_clock::now();
}

void FPS_Cap::limit() {
  auto frame_end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> frame_time =
      frame_end - _frame_start;

  // Sleep if frame_time too small.
  if (frame_time < _frame_duration) {
    std::this_thread::sleep_for(_frame_duration - frame_time);
  }

  // Resets timer for next frame.
  _frame_start = std::chrono::high_resolution_clock::now();
}