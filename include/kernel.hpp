#pragma once
#include <string>
#define R(...)                                                                 \
  std::string(" " #__VA_ARGS__ " ") // evil stringification macro, similar
                                    // syntax to raw string R"(...)"

// Kernel code to handle ball compute.
const std::string kernel_source();