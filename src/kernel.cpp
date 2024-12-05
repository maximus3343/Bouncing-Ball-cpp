#include "../include/kernel.hpp"

const std::string kernel_source =
    R"(
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

    __kernel void update_pos(__global Ball* balls, const int num_balls) {
        int id = get_global_id(0);

        if (id >= num_balls) return;

        balls[id].x += balls[id].vx;
        balls[id].y += balls[id].vy;

    }

    __kernel void handle_wall_colls(__global Ball* balls, const int num_balls){
        int global_id = get_global_id(0);
        int local_id = get_local_id(0);
        // Get ball index associated with work-item.
        int ball_idx = global_id / 4;

        if(ball_idx > num_balls) return;

        // Each work_items performs an if condition.
        switch(local_id){
        // Upper and lower walls.
            case 0:
                // Check for collisions.
                if((balls[ball_idx].y - balls[ball_idx].radius) < -1.0f){
                    balls[ball_idx].vy = -balls[ball_idx].vy;
                    balls[ball_idx].y = -1.0f + balls[ball_idx].radius;
                }
                // If no collision at bottom, can update with gravity.
                else{
                    balls[ball_idx].vy += balls[ball_idx].gravity;
                }
                break;
            case 1:
                if((balls[ball_idx].y + balls[ball_idx].radius) > 1.0f){
                    balls[ball_idx].vy = -balls[ball_idx].vy;
                    balls[ball_idx].y = 1.0f - balls[ball_idx].radius;
                }
                break;
        // Left and right walls.
            case 2:
                if((balls[ball_idx].x - balls[ball_idx].radius) < -1.0f){
                    balls[ball_idx].vx = -balls[ball_idx].vx;
                    balls[ball_idx].x = -1.0f + balls[ball_idx].radius;
                }
                break;
            case 3:
                if((balls[ball_idx].x + balls[ball_idx].radius) > 1.0f){
                    balls[ball_idx].vx = -balls[ball_idx].vx;
                    balls[ball_idx].x = 1.0f - balls[ball_idx].radius;
                }
                break;
        }
    }

    __kernel void handle_ball_colls(__global Ball *balls, const int num_balls){
        int global_id = get_global_id(0);
        int local_id = get_local_id(0);
        int local_size = get_local_size(0);

        if (global_id >= num_balls) return;

        // Used when collisions b/w two balls.
        __local Ball local_balls[2];

        for(int j = global_id + 1; j < num_balls; j++){
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
                float unit_normal[2] = {dx/distance, dy/distance};
                const float pen_depth = radiusSum - distance;
                // Correction to apply in x and y coordinates to both balls.
                const float correction[2] = {unit_normal[0]*( pen_depth/2),
                                            unit_normal[1] * (pen_depth/2)};

                local_balls[0].x += correction[0];
                local_balls[1].y += correction[1];
                local_balls[1].x -= correction[0];
                local_balls[1].y -= correction[1];

                // Perform elastic collision.
                // Simple speed exchange. Does not implicate mass.
                float temp_vx = local_balls[0].vx;
                float temp_vy = local_balls[0].vy;

                local_balls[0].vx = local_balls[1].vx;
                local_balls[0].vy = local_balls[1].vy;
                local_balls[1].vx = temp_vx;
                local_balls[1].vy = temp_vy;

                balls[global_id] = local_balls[0];
                balls[j] = local_balls[1];
            }
        }
    }

    __kernel void compute_ball_vertices(
    __global const Ball* balls,  // All balls.
    __global float2* vertices,         // Every vertices for all balls stored (x, y).
    const int num_balls,
    const int num_segments) {
        // Get the global work-item index (ball index)
        int ball_id = get_global_id(0);

        if (ball_id >= num_balls) return;  // Out of bounds check

        // Fetch ball data (position and radius) given global_id.
        float2 position = (float2)(balls[ball_id].x, balls[ball_id].y);
        float radius = balls[ball_id].radius;  // Assuming y stores the radius

        // Generate vertices for the ball.
        int vertex_count = 0;

        for (int i = 0; i < num_segments; i++) {
            // Angle for each segment
            float theta = (float)i / (num_segments - 1) * 6.28318f;  // 0 to 2π

            // Compute the x, y coordinates in polar form
            float x = radius * cos(theta);
            float y = radius * sin(theta);

            // Transform the vertex position to the circle's center
            float2 vertex = (float2)(x + position.x, y + position.y);

            // Store the computed vertex in the output buffer
            // Vertex count is local to each ball, hence we have contiguous
            // vertices for each ball.
            // Stride of num_segments necessary.
            vertices[ball_id * num_segments + vertex_count] = vertex;
            vertex_count++;
        }

    }
)";