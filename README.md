# Boucing-Ball-cpp
## Overview

This project is a simulation of bouncing balls using OpenGL for rendering and OpenCL for parallel computation of ball physics. An arbitrary number of balls can be spawned with random initial positions and velocities, and they interact with each other and the boundaries of the simulation space. The elastic collision detections and responses are computed on the GPU using OpenCL, allowing for efficient handling of multiple balls on the GPU.

## Features

+ Spawn an arbitrary number of balls with random initial positions and velocities.
+ Realistic gravity effect on the balls.
+ Balls bounce off each other and the boundaries of the simulation space.
+ Collision computations are performed on the GPU using OpenCL.
+ No synchronization between host and GPU, ensuring high performance.

## Requirements

+ OpenGL (for rendering)
+ OpenCL (for GPU computations)

+ You can run the program with the following command-line arguments:

- `--balls` or `-b`: Specify the number of balls.
- `--vertices` or `-v`: Specify the number of vertices.

## Default Values

If no arguments are provided, the program will use the following default values:
- **Number of balls**: 5
- **Number of vertices**: 40

## Example

To run the program with 10 balls and 100 vertices, use:

```bash
cd build
cmake .. && make && ./main --balls 10 --vertices 100
