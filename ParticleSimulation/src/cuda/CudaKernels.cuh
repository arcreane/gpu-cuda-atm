#pragma once

#include "CudaUtils.cuh"
#include "CollisionGrid.cuh"

// Kernel declarations
__global__ void applyForcesKernel(float* vx, float* vy, float gravity, float friction, int count, float dt);

__global__ void applyMouseForceKernel(float* x, float* y, float* vx, float* vy,
                                      float mouseX, float mouseY, float mouseVX, float mouseVY,
                                      float forceRadius, float forceStrength,
                                      bool leftButton, bool rightButton,
                                      int count, float dt);

__global__ void updatePositionsKernel(float* x, float* y, float* vx, float* vy, int count, float dt);

__global__ void handleWallCollisionsKernel(float* x, float* y, float* vx, float* vy, float* radius,
                                           float width, float height, float elasticity, int count);

__global__ void handleParticleCollisionsKernel(float* x, float* y, float* vx, float* vy,
                                               float* radius, float* mass,
                                               int* cellStart, int* cellEnd, int* particleIndex,
                                               int gridWidth, int gridHeight, float cellSize,
                                               float elasticity, int count, int* collisionCount);

// Host functions to launch kernels
void launchApplyForces(DeviceParticles& particles, const DeviceSimParams& params, float dt);
void launchApplyMouseForce(DeviceParticles& particles, const DeviceSimParams& params, float dt);
void launchUpdatePositions(DeviceParticles& particles, float dt);
void launchWallCollisions(DeviceParticles& particles, const DeviceSimParams& params);
void launchParticleCollisions(DeviceParticles& particles, DeviceGrid& grid,
                              const DeviceSimParams& params, int* d_collisionCount);
