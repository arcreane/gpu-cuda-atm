#include "CollisionGrid.cuh"
#include <thrust/sort.h>
#include <thrust/device_ptr.h>

void allocateGrid(DeviceGrid& grid, int gridWidth, int gridHeight, int maxParticles) {
    grid.gridWidth = gridWidth;
    grid.gridHeight = gridHeight;
    grid.cellCount = gridWidth * gridHeight;

    CUDA_CHECK(cudaMalloc(&grid.cellStart, grid.cellCount * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&grid.cellEnd, grid.cellCount * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&grid.particleIndex, maxParticles * sizeof(int)));
}

void freeGrid(DeviceGrid& grid) {
    if (grid.cellStart) cudaFree(grid.cellStart);
    if (grid.cellEnd) cudaFree(grid.cellEnd);
    if (grid.particleIndex) cudaFree(grid.particleIndex);
    grid.cellStart = nullptr;
    grid.cellEnd = nullptr;
    grid.particleIndex = nullptr;
}

__global__ void calcHashKernel(float* x, float* y, int* cellIndex, int* particleIndex,
                               int count, float cellSize, int gridWidth, int gridHeight) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    int cellX = min(max(int(x[i] / cellSize), 0), gridWidth - 1);
    int cellY = min(max(int(y[i] / cellSize), 0), gridHeight - 1);

    cellIndex[i] = cellY * gridWidth + cellX;
    particleIndex[i] = i;
}

__global__ void findCellStartEndKernel(int* cellIndex, int* cellStart, int* cellEnd, int count) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    int cell = cellIndex[i];

    if (i == 0) {
        cellStart[cell] = 0;
    } else {
        int prevCell = cellIndex[i - 1];
        if (cell != prevCell) {
            cellStart[cell] = i;
            cellEnd[prevCell] = i;
        }
    }

    if (i == count - 1) {
        cellEnd[cell] = count;
    }
}

void buildGrid(DeviceParticles& particles, DeviceGrid& grid, float cellSize, int gridWidth, int gridHeight) {
    int count = particles.count;
    if (count == 0) return;

    int blockSize = 256;
    int numBlocks = (count + blockSize - 1) / blockSize;

    // Allocate temporary array for cell indices
    int* d_cellIndex;
    CUDA_CHECK(cudaMalloc(&d_cellIndex, count * sizeof(int)));

    // Reset cell start/end
    CUDA_CHECK(cudaMemset(grid.cellStart, 0xFF, grid.cellCount * sizeof(int))); // Set to -1
    CUDA_CHECK(cudaMemset(grid.cellEnd, 0, grid.cellCount * sizeof(int)));

    // Calculate hash for each particle
    calcHashKernel<<<numBlocks, blockSize>>>(
        particles.x, particles.y, d_cellIndex, grid.particleIndex,
        count, cellSize, gridWidth, gridHeight
    );

    // Sort by cell index using Thrust
    thrust::device_ptr<int> d_cellIndex_ptr(d_cellIndex);
    thrust::device_ptr<int> d_particleIndex_ptr(grid.particleIndex);
    thrust::sort_by_key(d_cellIndex_ptr, d_cellIndex_ptr + count, d_particleIndex_ptr);

    // Find cell boundaries
    findCellStartEndKernel<<<numBlocks, blockSize>>>(d_cellIndex, grid.cellStart, grid.cellEnd, count);

    cudaFree(d_cellIndex);
}
