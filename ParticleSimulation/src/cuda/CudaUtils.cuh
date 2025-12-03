#pragma once

#include <cuda_runtime.h>
#include <cstdio>

// CUDA error checking macro
#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d - %s\n", \
                    __FILE__, __LINE__, cudaGetErrorString(error)); \
        } \
    } while(0)

// Device particle structure (SOA for better memory coalescing)
struct DeviceParticles {
    float* x;
    float* y;
    float* vx;
    float* vy;
    float* radius;
    float* mass;
    unsigned char* r;
    unsigned char* g;
    unsigned char* b;
    unsigned char* a;
    int count;
};

// Simulation parameters for GPU
struct DeviceSimParams {
    float gravity;
    float friction;
    float elasticity;
    float mouseX;
    float mouseY;
    float mouseVX;
    float mouseVY;
    float mouseForceRadius;
    float mouseForceStrength;
    bool mouseInArea;
    bool mouseLeftButton;
    bool mouseRightButton;
    float width;
    float height;
};

// Grid for spatial partitioning
struct DeviceGrid {
    int* cellStart;
    int* cellEnd;
    int* particleIndex;
    int cellCount;
    int gridWidth;
    int gridHeight;
    float cellSize;
};

// Utility functions
bool initCuda();
void cleanupCuda();
bool isCudaAvailable();
