#include "GPUPhysicsEngine.h"
#include "cuda/CudaKernels.cuh"
#include "cuda/CudaUtils.cuh"
#include "cuda/CollisionGrid.cuh"
#include "SimulatorConfig.h"

GPUPhysicsEngine::GPUPhysicsEngine() {
    m_cudaAvailable = isCudaAvailable();
    if (m_cudaAvailable) {
        initCuda();
        m_deviceParticles = new DeviceParticles();
        m_deviceGrid = new DeviceGrid();
        memset(m_deviceParticles, 0, sizeof(DeviceParticles));
        memset(m_deviceGrid, 0, sizeof(DeviceGrid));
    }
}

GPUPhysicsEngine::~GPUPhysicsEngine() {
    cleanup();
    if (m_deviceParticles) delete m_deviceParticles;
    if (m_deviceGrid) delete m_deviceGrid;
}

bool GPUPhysicsEngine::isAvailable() const {
    return m_cudaAvailable;
}

void GPUPhysicsEngine::allocateDeviceMemory(int count) {
    if (count <= 0) return;

    // Free previous allocation if different size
    if (m_allocatedCount != count) {
        freeDeviceMemory();
    }

    if (m_allocatedCount == count) return;

    // Allocate particle arrays
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->x, count * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->y, count * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->vx, count * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->vy, count * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->radius, count * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->mass, count * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->r, count * sizeof(unsigned char)));
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->g, count * sizeof(unsigned char)));
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->b, count * sizeof(unsigned char)));
    CUDA_CHECK(cudaMalloc(&m_deviceParticles->a, count * sizeof(unsigned char)));

    m_deviceParticles->count = count;

    // Allocate collision counter
    CUDA_CHECK(cudaMalloc(&m_d_collisionCount, sizeof(int)));

    // Allocate grid
    int gridWidth = static_cast<int>(std::ceil(Config::RENDER_WIDTH / (float)Config::GRID_CELL_SIZE));
    int gridHeight = static_cast<int>(std::ceil(Config::RENDER_HEIGHT / (float)Config::GRID_CELL_SIZE));
    m_deviceGrid->cellSize = Config::GRID_CELL_SIZE;
    allocateGrid(*m_deviceGrid, gridWidth, gridHeight, count);

    m_allocatedCount = count;
}

void GPUPhysicsEngine::freeDeviceMemory() {
    if (m_deviceParticles->x) cudaFree(m_deviceParticles->x);
    if (m_deviceParticles->y) cudaFree(m_deviceParticles->y);
    if (m_deviceParticles->vx) cudaFree(m_deviceParticles->vx);
    if (m_deviceParticles->vy) cudaFree(m_deviceParticles->vy);
    if (m_deviceParticles->radius) cudaFree(m_deviceParticles->radius);
    if (m_deviceParticles->mass) cudaFree(m_deviceParticles->mass);
    if (m_deviceParticles->r) cudaFree(m_deviceParticles->r);
    if (m_deviceParticles->g) cudaFree(m_deviceParticles->g);
    if (m_deviceParticles->b) cudaFree(m_deviceParticles->b);
    if (m_deviceParticles->a) cudaFree(m_deviceParticles->a);

    memset(m_deviceParticles, 0, sizeof(DeviceParticles));

    if (m_d_collisionCount) cudaFree(m_d_collisionCount);
    m_d_collisionCount = nullptr;

    freeGrid(*m_deviceGrid);

    m_allocatedCount = 0;
}

void GPUPhysicsEngine::copyToDevice(const std::vector<Particle>& particles) {
    int count = static_cast<int>(particles.size());
    if (count == 0) return;

    // Create temporary arrays for SOA layout
    std::vector<float> x(count), y(count), vx(count), vy(count), radius(count), mass(count);
    std::vector<unsigned char> r(count), g(count), b(count), a(count);

    for (int i = 0; i < count; ++i) {
        x[i] = particles[i].x;
        y[i] = particles[i].y;
        vx[i] = particles[i].vx;
        vy[i] = particles[i].vy;
        radius[i] = particles[i].radius;
        mass[i] = particles[i].mass;
        r[i] = particles[i].r;
        g[i] = particles[i].g;
        b[i] = particles[i].b;
        a[i] = particles[i].a;
    }

    // Copy to device
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->x, x.data(), count * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->y, y.data(), count * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->vx, vx.data(), count * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->vy, vy.data(), count * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->radius, radius.data(), count * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->mass, mass.data(), count * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->r, r.data(), count * sizeof(unsigned char), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->g, g.data(), count * sizeof(unsigned char), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->b, b.data(), count * sizeof(unsigned char), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_deviceParticles->a, a.data(), count * sizeof(unsigned char), cudaMemcpyHostToDevice));
}

void GPUPhysicsEngine::copyFromDevice(std::vector<Particle>& particles) {
    int count = static_cast<int>(particles.size());
    if (count == 0) return;

    // Create temporary arrays
    std::vector<float> x(count), y(count), vx(count), vy(count);

    // Copy from device (only position and velocity need to be copied back)
    CUDA_CHECK(cudaMemcpy(x.data(), m_deviceParticles->x, count * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(y.data(), m_deviceParticles->y, count * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(vx.data(), m_deviceParticles->vx, count * sizeof(float), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(vy.data(), m_deviceParticles->vy, count * sizeof(float), cudaMemcpyDeviceToHost));

    // Update particles
    for (int i = 0; i < count; ++i) {
        particles[i].x = x[i];
        particles[i].y = y[i];
        particles[i].vx = vx[i];
        particles[i].vy = vy[i];
    }
}

void GPUPhysicsEngine::initialize(std::vector<Particle>& particles) {
    if (!m_cudaAvailable) return;

    int count = static_cast<int>(particles.size());
    allocateDeviceMemory(count);
    copyToDevice(particles);
    m_initialized = true;
}

void GPUPhysicsEngine::update(std::vector<Particle>& particles,
                              const SimulationParameters& params,
                              const MouseState& mouse,
                              float deltaTime) {
    if (!m_cudaAvailable || !m_initialized) return;

    int count = static_cast<int>(particles.size());
    if (count == 0) return;

    // Prepare simulation parameters
    DeviceSimParams deviceParams;
    deviceParams.gravity = params.gravity;
    deviceParams.friction = params.friction;
    deviceParams.elasticity = params.elasticity;
    deviceParams.mouseX = mouse.x;
    deviceParams.mouseY = mouse.y;
    deviceParams.mouseVX = mouse.vx;
    deviceParams.mouseVY = mouse.vy;
    deviceParams.mouseForceRadius = params.mouseForceRadius;
    deviceParams.mouseForceStrength = params.mouseForceStrength;
    deviceParams.mouseInArea = mouse.isInRenderArea;
    deviceParams.mouseLeftButton = mouse.leftButtonDown;
    deviceParams.mouseRightButton = mouse.rightButtonDown;
    deviceParams.width = Config::RENDER_WIDTH;
    deviceParams.height = Config::RENDER_HEIGHT;

    // Reset collision counter
    CUDA_CHECK(cudaMemset(m_d_collisionCount, 0, sizeof(int)));

    // Apply forces
    launchApplyForces(*m_deviceParticles, deviceParams, deltaTime);

    // Apply mouse force
    launchApplyMouseForce(*m_deviceParticles, deviceParams, deltaTime);

    // Update positions
    launchUpdatePositions(*m_deviceParticles, deltaTime);

    // Wall collisions
    launchWallCollisions(*m_deviceParticles, deviceParams);

    // Build spatial grid
    buildGrid(*m_deviceParticles, *m_deviceGrid, m_deviceGrid->cellSize,
              m_deviceGrid->gridWidth, m_deviceGrid->gridHeight);

    // Particle collisions
    launchParticleCollisions(*m_deviceParticles, *m_deviceGrid, deviceParams, m_d_collisionCount);

    // Synchronize
    CUDA_CHECK(cudaDeviceSynchronize());

    // Get collision count
    CUDA_CHECK(cudaMemcpy(&m_collisionCount, m_d_collisionCount, sizeof(int), cudaMemcpyDeviceToHost));

    // Copy results back to host
    copyFromDevice(particles);
}

void GPUPhysicsEngine::cleanup() {
    if (!m_cudaAvailable) return;
    freeDeviceMemory();
    m_initialized = false;
}
