#include "../include/kernel.hpp"

// tbb pipeline with ball program.

const std::string kernel_source() {
  return R(
      // Must redefine the Ball struct inside the Kernel source code.
      typedef struct {
        float x;
        float y;
        float vx;
        float vy;
        float radius;
        float mass;
        float gravity;
        float colors[3];
      } Ball;

      __kernel void update_pos(__global Ball * balls, const int num_balls) {
        int id = get_global_id(0);

        if (id >= num_balls)
          return;

        balls[id].x += balls[id].vx;
        balls[id].y += balls[id].vy;
      }

      __kernel void handle_wall_colls(__global Ball * balls,
                                      const int num_balls) {
        int global_id = get_global_id(0);
        int local_id = get_local_id(0);
        // Get ball index associated with work-item.
        int ball_idx = global_id / 4;

        if (ball_idx > num_balls)
          return;

        const float y = balls[ball_idx].y;
        const float x = balls[ball_idx].x;
        const float radius = balls[ball_idx].radius;

        // Each work_items performs an if condition.
        switch (local_id) {
          // Continuous collision detection for upper and lower walls.
        case 0: // Bottom boundary.
          // Check for collisions.
          if ((y - radius) < -1.0f) {
            // First calculate the exact time at which collisions occurs.
            const float gravity = balls[ball_idx].gravity;
            const float vy0 = balls[ball_idx].vy; // Initial speed.
            // For the duration of the frame, the acceleration is non-existent.
            const float y0 =
                balls[ball_idx].y - vy0 -
                balls[ball_idx]
                    .radius; // Position of bottom Ball before collision.
            const float time =
                (-y0 - 1) / vy0; // Exact time of collision [0,1];

            // But we will update the ball speed according to its acceleration.
            balls[ball_idx].vy =
                -(vy0 - gravity * time); // Continuous collision detection.
            balls[ball_idx].y =
                -1.0f + balls[ball_idx].radius; // Rectify position of ball.
          }
          // If no collision at bottom, can update with gravity.
          else {
            balls[ball_idx].vy += balls[ball_idx].gravity;
          }
          break;
        case 1: // Top boundary.
          if ((y + radius) > 1.0f) {
            // First calculate the exact time at which collisions occurs.
            const float gravity = balls[ball_idx].gravity;
            const float vy0 = balls[ball_idx].vy; // Initial speed.
            // For the duration of the frame, the acceleration is non-existent.
            const float y0 =
                balls[ball_idx].y - vy0 +
                balls[ball_idx]
                    .radius; // Position of bottom Ball before collision.
            const float time = (1 - y0) / vy0; // Exact time of collision [0,1]

            balls[ball_idx].vy = -(vy0 + gravity * time);
            balls[ball_idx].y = 1.0f - balls[ball_idx].radius;
          }
          break;
          // Left and right walls.
        case 2:
          if ((x - radius) < -1.0f) {
            balls[ball_idx].vx = -balls[ball_idx].vx;
            balls[ball_idx].x = -1.0f + balls[ball_idx].radius;
          }
          break;
        case 3:
          if ((x + radius) > 1.0f) {
            balls[ball_idx].vx = -balls[ball_idx].vx;
            balls[ball_idx].x = 1.0f - balls[ball_idx].radius;
          }
          break;
        }
      }

      __kernel void handle_ball_colls(__global Ball * balls,
                                      const int num_balls) {
        int global_id = get_global_id(0);
        int local_id = get_local_id(0);
        int local_size = get_local_size(0);

        if (global_id >= num_balls)
          return;

        // Used when collisions b/w two balls.
        __local Ball local_balls[2];

        for (int j = global_id + 1; j < num_balls; j++) {
          float dx = balls[global_id].x - balls[j].x;
          float dy = balls[global_id].y - balls[j].y;
          float distance = sqrt(dx * dx + dy * dy);
          float radiusSum = balls[global_id].radius + balls[j].radius;

          // We have a collision.
          if (distance < radiusSum) {
            // Load relevant balls into local memory for further processing.
            // Faster access time.
            local_balls[0] = balls[global_id];
            local_balls[1] = balls[j];

            // Corrects the overlapping between the balls colliding.
            float unit_normal[2] = {dx / distance, dy / distance};
            const float pen_depth = radiusSum - distance;
            // Correction to apply in x and y coordinates to both balls.
            const float correction[2] = {unit_normal[0] * (pen_depth / 2),
                                         unit_normal[1] * (pen_depth / 2)};

            local_balls[0].x += correction[0];
            local_balls[0].y += correction[1];
            local_balls[1].x -= correction[0];
            local_balls[1].y -= correction[1];

            // Perform elastic collision.
            // Simple speed exchange. Does not implicate mass.
            const float temp_vx = local_balls[0].vx;
            const float temp_vy = local_balls[0].vy;

            local_balls[0].vx = local_balls[1].vx;
            local_balls[0].vy = local_balls[1].vy;
            local_balls[1].vx = temp_vx;
            local_balls[1].vy = temp_vy;

            balls[global_id] = local_balls[0];
            balls[j] = local_balls[1];
          }
        }
      }

      // Compute the vertices of the ball, given its position, on the GPU.
      __kernel void compute_ball_vertices(
          __global const Ball *balls, // All balls.
          __global float3
              *vertices, // Every vertices for all balls stored (x, y, z).
          const int num_balls, const int num_segments) {
        // Get the global work-item index (ball index)
        const int ball_id = get_global_id(0);

        if (ball_id >= num_balls)
          return; // Out of bounds check

        // Fetch ball data (position and radius) given global_id.
        const float3 position =
            (float3)(balls[ball_id].x, balls[ball_id].y, 1.0f);
        const float radius = balls[ball_id].radius;

        // Generate vertices for the ball.
        int vertex_count = 1;

        // Set up the center vertex.
        vertices[ball_id * num_segments] = position;

        for (int i = 0; i < (num_segments - 1); i++) {
          // Angle for each segment.
          // Must reach angle of 2π else 2nd vertex and last vertex won't
          // connect.
          const float theta =
              (float)i / (num_segments - 2) * 6.28318f; // 0 to 2π inclusively.

          // Compute the x, y coordinates in polar form
          const float x = radius * cos(theta);
          const float y = radius * sin(theta);

          // Transform the vertex position to the circle's center
          const float3 vertex = (float3)(x + position.x, y + position.y, 1.0f);

          // Store the vertices for each ball stored consecutively in the
          // buffer.
          vertices[(ball_id * num_segments + vertex_count)] = vertex;
          vertex_count++;
        }
      }

      // For debugging only. Prints out the vertices position to check if
      // compute is done properly.
      // Prints only the first ball in the buffer.
      __kernel void print_vertices(__global float3 * vertices,
                                   const int num_balls,
                                   const int num_segments) {
        printf("Size of the float3: %d\n", sizeof(float3));
        for (int i = 0; i < num_segments; i++) {
          printf("We have vertex %d at: (%f, %f)\n", i, vertices[i].x,
                 vertices[i].y);
        }
      });
};