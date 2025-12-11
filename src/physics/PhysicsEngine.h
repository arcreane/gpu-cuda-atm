#pragma once

#include <vector>
#include "core/SimulationState.h"

class PhysicsEngine {
public:
    virtual ~PhysicsEngine() = default;

    virtual void initialize(std::vector<Particle>& particles) = 0;
    virtual void update(std::vector<Particle>& particles,
                       const SimulationParameters& params,
                       const MouseState& mouse,
                       float deltaTime) = 0;
    virtual void cleanup() = 0;

    virtual bool isAvailable() const = 0;
    virtual const char* getName() const = 0;

    virtual int getCollisionCount() const { return m_collisionCount; }

protected:
    int m_collisionCount = 0;
};
