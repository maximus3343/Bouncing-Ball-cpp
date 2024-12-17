# Boucing-Ball-cpp
**Overview**

This project is a simulation of bouncing balls using OpenGL for rendering and OpenCL for parallel computation of ball physics. An arbitrary number of balls can be spawned with random initial positions and velocities, and they interact with each other and the boundaries of the simulation space. The elastic collision detections and responses are computed on the GPU using OpenCL, allowing for efficient handling of multiple balls on the GPU.

**Features**

+ Spawn an arbitrary number of balls with random initial positions and velocities.
+ Realistic gravity effect on the balls.
+ Balls bounce off each other and the boundaries of the simulation space.
+ Collision computations are performed on the GPU using OpenCL.
+ No synchronization between host and GPU, ensuring high performance.
+ Command-line interface (CLI) for easy configuration of the simulation parameters.

**Requirements**

+ CMake (for building the project)
+ OpenGL (for rendering)
+ OpenCL (for GPU computations)
+ Compatible GPU with OpenCL support
+ C++ compiler (e.g., g++, clang++)


