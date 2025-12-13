#pragma once

#include <cstdint>

namespace Config {
    // Window settings
    constexpr int WINDOW_RENDER_WIDTH = 800;    // Logical window size
    constexpr int WINDOW_RENDER_HEIGHT = 600;
    constexpr int DPI = 3;			// DPI pixel density
    constexpr int SIMULATION_WIDTH = WINDOW_RENDER_WIDTH*DPI;
    constexpr int SIMULATION_HEIGHT = WINDOW_RENDER_HEIGHT*DPI;
    constexpr int UI_WIDTH = 300;
    constexpr int WINDOW_WIDTH = WINDOW_RENDER_WIDTH + UI_WIDTH;
    constexpr int WINDOW_HEIGHT = WINDOW_RENDER_HEIGHT;

    // Scaling factor for rendering
    constexpr float RENDER_SCALE_X = static_cast<float>(WINDOW_RENDER_WIDTH) / SIMULATION_WIDTH;
    constexpr float RENDER_SCALE_Y = static_cast<float>(WINDOW_RENDER_HEIGHT) / SIMULATION_HEIGHT;

    // Legacy compatibility
    constexpr int RENDER_WIDTH = WINDOW_RENDER_WIDTH;
    constexpr int RENDER_HEIGHT = WINDOW_RENDER_HEIGHT;

    // Simulation defaults
    constexpr int DEFAULT_PARTICLE_COUNT = 1000;
    constexpr int MIN_PARTICLES = 100;
    constexpr int MAX_PARTICLES = 50000;

    constexpr float DEFAULT_MIN_RADIUS = 3.0f;
    constexpr float DEFAULT_MAX_RADIUS = 8.0f;
    constexpr float MIN_RADIUS = 2.0f;
    constexpr float MAX_RADIUS = 15.0f;

    constexpr float DEFAULT_MIN_VELOCITY = 0.0f;
    constexpr float DEFAULT_MAX_VELOCITY = 100.0f;
    constexpr float MAX_VELOCITY = 200.0f;

    constexpr float DEFAULT_ELASTICITY = 0.8f;
    constexpr float DEFAULT_FRICTION = 0.02f;
    constexpr float DEFAULT_GRAVITY = 0.0f;

    // Mouse interaction
    constexpr float MOUSE_FORCE_RADIUS = 100.0f;
    constexpr float MOUSE_FORCE_STRENGTH = 5000.0f;

    // CUDA settings
    constexpr int CUDA_BLOCK_SIZE = 256;
    constexpr int GRID_CELL_SIZE = 32;

    // Physics
    constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;
    constexpr int COLLISION_ITERATIONS = 2;
}
