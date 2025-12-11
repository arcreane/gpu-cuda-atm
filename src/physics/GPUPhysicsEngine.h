#pragma once

#include "PhysicsEngine.h"

struct DeviceParticles;
struct DeviceGrid;

class GPUPhysicsEngine : public PhysicsEngine {
public:
    GPUPhysicsEngine();
    ~GPUPhysicsEngine() override;

    void initialize(std::vector<Particle>& particles) override;
    void update(std::vector<Particle>& particles,
               const SimulationParameters& params,
               const MouseState& mouse,
               float deltaTime) override;
    void cleanup() override;

    bool isAvailable() const override;
    const char* getName() const override { return "GPU (CUDA)"; }

private:
    void allocateDeviceMemory(int count);
    void freeDeviceMemory();
    void copyToDevice(const std::vector<Particle>& particles);
    void copyFromDevice(std::vector<Particle>& particles);

    DeviceParticles* m_deviceParticles = nullptr;
    DeviceGrid* m_deviceGrid = nullptr;
    int* m_d_collisionCount = nullptr;

    bool m_initialized = false;
    bool m_cudaAvailable = false;
    int m_allocatedCount = 0;
};
