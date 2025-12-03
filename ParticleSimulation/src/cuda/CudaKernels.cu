#include "CudaKernels.cuh"
#include "SimulatorConfig.h"

__global__ void applyForcesKernel(float* vx, float* vy, float gravity, float friction, int count, float dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    // Apply gravity
    vy[i] += gravity * dt;

    // Apply friction
    float frictionFactor = 1.0f - friction;
    vx[i] *= frictionFactor;
    vy[i] *= frictionFactor;
}

__global__ void applyMouseForceKernel(float* x, float* y, float* vx, float* vy,
                                      float mouseX, float mouseY, float mouseVX, float mouseVY,
                                      float forceRadius, float forceStrength,
                                      bool leftButton, bool rightButton,
                                      int count, float dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    float dx = x[i] - mouseX;
    float dy = y[i] - mouseY;
    float distSq = dx * dx + dy * dy;
    float radiusSq = forceRadius * forceRadius;

    if (distSq < radiusSq && distSq > 0.001f) {
        float dist = sqrtf(distSq);
        float strength = (1.0f - dist / forceRadius);

        float nx = dx / dist;
        float ny = dy / dist;

        float forceMagnitude = forceStrength * strength * dt;

        if (leftButton) {
            // Attraction
            vx[i] -= nx * forceMagnitude;
            vy[i] -= ny * forceMagnitude;
        } else if (rightButton) {
            // Repulsion
            vx[i] += nx * forceMagnitude * 2.0f;
            vy[i] += ny * forceMagnitude * 2.0f;
        } else {
            // Mouse movement influence
            float mouseSpeed = sqrtf(mouseVX * mouseVX + mouseVY * mouseVY);
            if (mouseSpeed > 1.0f) {
                vx[i] += mouseVX * strength * 0.5f;
                vy[i] += mouseVY * strength * 0.5f;
            }
        }
    }
}

__global__ void updatePositionsKernel(float* x, float* y, float* vx, float* vy, int count, float dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    x[i] += vx[i] * dt;
    y[i] += vy[i] * dt;
}

__global__ void handleWallCollisionsKernel(float* x, float* y, float* vx, float* vy, float* radius,
                                           float width, float height, float elasticity, int count) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    float r = radius[i];

    // Left wall
    if (x[i] - r < 0) {
        x[i] = r;
        vx[i] = -vx[i] * elasticity;
    }
    // Right wall
    if (x[i] + r > width) {
        x[i] = width - r;
        vx[i] = -vx[i] * elasticity;
    }
    // Top wall
    if (y[i] - r < 0) {
        y[i] = r;
        vy[i] = -vy[i] * elasticity;
    }
    // Bottom wall
    if (y[i] + r > height) {
        y[i] = height - r;
        vy[i] = -vy[i] * elasticity;
    }
}

__global__ void handleParticleCollisionsKernel(float* x, float* y, float* vx, float* vy,
                                               float* radius, float* mass,
                                               int* cellStart, int* cellEnd, int* particleIndex,
                                               int gridWidth, int gridHeight, float cellSize,
                                               float elasticity, int count, int* collisionCount) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count) return;

    int i = particleIndex[idx];

    float xi = x[i];
    float yi = y[i];
    float ri = radius[i];
    float mi = mass[i];
    float vxi = vx[i];
    float vyi = vy[i];

    int cellX = min(max(int(xi / cellSize), 0), gridWidth - 1);
    int cellY = min(max(int(yi / cellSize), 0), gridHeight - 1);

    // Check neighboring cells
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = cellX + dx;
            int ny = cellY + dy;

            if (nx < 0 || nx >= gridWidth || ny < 0 || ny >= gridHeight)
                continue;

            int cellIdx = ny * gridWidth + nx;
            int start = cellStart[cellIdx];
            int end = cellEnd[cellIdx];

            if (start == -1) continue;

            for (int k = start; k < end; ++k) {
                int j = particleIndex[k];
                if (j <= i) continue;

                float dxp = x[j] - xi;
                float dyp = y[j] - yi;
                float distSq = dxp * dxp + dyp * dyp;
                float minDist = ri + radius[j];

                if (distSq < minDist * minDist && distSq > 0.0001f) {
                    float dist = sqrtf(distSq);
                    float overlap = minDist - dist;

                    float nx = dxp / dist;
                    float ny = dyp / dist;

                    float mj = mass[j];
                    float totalMass = mi + mj;
                    float p1Ratio = mj / totalMass;
                    float p2Ratio = mi / totalMass;

                    // Separate particles
                    atomicAdd(&x[i], -nx * overlap * p1Ratio);
                    atomicAdd(&y[i], -ny * overlap * p1Ratio);
                    atomicAdd(&x[j], nx * overlap * p2Ratio);
                    atomicAdd(&y[j], ny * overlap * p2Ratio);

                    // Calculate relative velocity
                    float relVelX = vxi - vx[j];
                    float relVelY = vyi - vy[j];
                    float relVelN = relVelX * nx + relVelY * ny;

                    if (relVelN > 0) {
                        float impulse = (1.0f + elasticity) * relVelN / totalMass;

                        atomicAdd(&vx[i], -impulse * mj * nx);
                        atomicAdd(&vy[i], -impulse * mj * ny);
                        atomicAdd(&vx[j], impulse * mi * nx);
                        atomicAdd(&vy[j], impulse * mi * ny);

                        atomicAdd(collisionCount, 1);
                    }
                }
            }
        }
    }
}

// Host launch functions
void launchApplyForces(DeviceParticles& particles, const DeviceSimParams& params, float dt) {
    int blockSize = Config::CUDA_BLOCK_SIZE;
    int numBlocks = (particles.count + blockSize - 1) / blockSize;
    applyForcesKernel<<<numBlocks, blockSize>>>(
        particles.vx, particles.vy, params.gravity, params.friction, particles.count, dt
    );
}

void launchApplyMouseForce(DeviceParticles& particles, const DeviceSimParams& params, float dt) {
    if (!params.mouseInArea) return;

    int blockSize = Config::CUDA_BLOCK_SIZE;
    int numBlocks = (particles.count + blockSize - 1) / blockSize;
    applyMouseForceKernel<<<numBlocks, blockSize>>>(
        particles.x, particles.y, particles.vx, particles.vy,
        params.mouseX, params.mouseY, params.mouseVX, params.mouseVY,
        params.mouseForceRadius, params.mouseForceStrength,
        params.mouseLeftButton, params.mouseRightButton,
        particles.count, dt
    );
}

void launchUpdatePositions(DeviceParticles& particles, float dt) {
    int blockSize = Config::CUDA_BLOCK_SIZE;
    int numBlocks = (particles.count + blockSize - 1) / blockSize;
    updatePositionsKernel<<<numBlocks, blockSize>>>(
        particles.x, particles.y, particles.vx, particles.vy, particles.count, dt
    );
}

void launchWallCollisions(DeviceParticles& particles, const DeviceSimParams& params) {
    int blockSize = Config::CUDA_BLOCK_SIZE;
    int numBlocks = (particles.count + blockSize - 1) / blockSize;
    handleWallCollisionsKernel<<<numBlocks, blockSize>>>(
        particles.x, particles.y, particles.vx, particles.vy, particles.radius,
        params.width, params.height, params.elasticity, particles.count
    );
}

void launchParticleCollisions(DeviceParticles& particles, DeviceGrid& grid,
                              const DeviceSimParams& params, int* d_collisionCount) {
    int blockSize = Config::CUDA_BLOCK_SIZE;
    int numBlocks = (particles.count + blockSize - 1) / blockSize;
    handleParticleCollisionsKernel<<<numBlocks, blockSize>>>(
        particles.x, particles.y, particles.vx, particles.vy,
        particles.radius, particles.mass,
        grid.cellStart, grid.cellEnd, grid.particleIndex,
        grid.gridWidth, grid.gridHeight, grid.cellSize,
        params.elasticity, particles.count, d_collisionCount
    );
}
