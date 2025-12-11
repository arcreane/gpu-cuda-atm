#pragma once

#include "CudaUtils.cuh"

// Initialize grid on device
void allocateGrid(DeviceGrid& grid, int gridWidth, int gridHeight, int maxParticles);

// Free grid memory
void freeGrid(DeviceGrid& grid);

// Build spatial hash grid
void buildGrid(DeviceParticles& particles, DeviceGrid& grid, float cellSize, int gridWidth, int gridHeight);

// Kernels for grid building
__global__ void calcHashKernel(float* x, float* y, int* cellIndex, int* particleIndex,
                               int count, float cellSize, int gridWidth, int gridHeight);

__global__ void findCellStartEndKernel(int* cellIndex, int* cellStart, int* cellEnd, int count);
