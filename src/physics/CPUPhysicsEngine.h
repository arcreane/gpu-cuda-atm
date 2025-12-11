#pragma once

#include "PhysicsEngine.h"
#include <vector>

class CPUPhysicsEngine : public PhysicsEngine {
public:
    CPUPhysicsEngine();
    ~CPUPhysicsEngine() override;

    void initialize(std::vector<Particle>& particles) override;
    void update(std::vector<Particle>& particles,
               const SimulationParameters& params,
               const MouseState& mouse,
               float deltaTime) override;
    void cleanup() override;

    bool isAvailable() const override { return true; }
    const char* getName() const override { return "CPU"; }

private:
    void applyForces(std::vector<Particle>& particles,
                    const SimulationParameters& params,
                    float deltaTime);
    void applyMouseForce(std::vector<Particle>& particles,
                        const MouseState& mouse,
                        const SimulationParameters& params,
                        float deltaTime);
    void updatePositions(std::vector<Particle>& particles, float deltaTime);
    void handleWallCollisions(std::vector<Particle>& particles,
                             const SimulationParameters& params,
                             float width, float height);
    void handleParticleCollisions(std::vector<Particle>& particles,
                                 const SimulationParameters& params);

    // Spatial grid for collision optimization
    std::vector<std::vector<int>> m_grid;
    int m_gridWidth = 0;
    int m_gridHeight = 0;
    float m_cellSize = 0.0f;
};
