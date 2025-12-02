#include "CPUPhysicsEngine.h"
#include "SimulatorConfig.h"
#include <cmath>
#include <algorithm>

CPUPhysicsEngine::CPUPhysicsEngine() = default;
CPUPhysicsEngine::~CPUPhysicsEngine() = default;

void CPUPhysicsEngine::initialize(std::vector<Particle>& particles) {
    // Initialize spatial grid
    m_cellSize = Config::GRID_CELL_SIZE;
    m_gridWidth = static_cast<int>(std::ceil(Config::SIMULATION_WIDTH / m_cellSize));
    m_gridHeight = static_cast<int>(std::ceil(Config::SIMULATION_HEIGHT / m_cellSize));
    m_grid.resize(m_gridWidth * m_gridHeight);
}

void CPUPhysicsEngine::cleanup() {
    m_grid.clear();
}

void CPUPhysicsEngine::update(std::vector<Particle>& particles,
                              const SimulationParameters& params,
                              const MouseState& mouse,
                              float deltaTime) {
    m_collisionCount = 0;

    // Apply forces (gravity, friction)
    applyForces(particles, params, deltaTime);

    // Apply mouse interaction
    if (mouse.isInRenderArea) {
        applyMouseForce(particles, mouse, params, deltaTime);
    }

    // Update positions
    updatePositions(particles, deltaTime);

    // Handle collisions
    handleWallCollisions(particles, params, Config::SIMULATION_WIDTH, Config::SIMULATION_HEIGHT);
    handleParticleCollisions(particles, params);
}

void CPUPhysicsEngine::applyForces(std::vector<Particle>& particles,
                                   const SimulationParameters& params,
                                   float deltaTime) {
    for (auto& p : particles) {
        // Apply gravity
        p.vy += params.gravity * deltaTime;

        // Apply friction (viscous drag)
        float frictionFactor = 1.0f - params.friction;
        p.vx *= frictionFactor;
        p.vy *= frictionFactor;
    }
}

void CPUPhysicsEngine::applyMouseForce(std::vector<Particle>& particles,
                                       const MouseState& mouse,
                                       const SimulationParameters& params,
                                       float deltaTime) {
    float radiusSq = params.mouseForceRadius * params.mouseForceRadius;

    for (auto& p : particles) {
        float dx = p.x - mouse.x;
        float dy = p.y - mouse.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < radiusSq && distSq > 0.001f) {
            float dist = std::sqrt(distSq);
            float strength = (1.0f - dist / params.mouseForceRadius);

            // Normalize direction
            float nx = dx / dist;
            float ny = dy / dist;

            // Calculate force based on mouse velocity and button state
            float forceMagnitude = params.mouseForceStrength * strength * deltaTime;

            if (mouse.leftButtonDown) {
                // Attraction
                p.vx -= nx * forceMagnitude;
                p.vy -= ny * forceMagnitude;
            } else if (mouse.rightButtonDown) {
                // Repulsion (explosion)
                p.vx += nx * forceMagnitude * 2.0f;
                p.vy += ny * forceMagnitude * 2.0f;
            } else {
                // Mouse movement influence
                float mouseSpeed = std::sqrt(mouse.vx * mouse.vx + mouse.vy * mouse.vy);
                if (mouseSpeed > 1.0f) {
                    p.vx += mouse.vx * strength * 0.5f;
                    p.vy += mouse.vy * strength * 0.5f;
                }
            }
        }
    }
}

void CPUPhysicsEngine::updatePositions(std::vector<Particle>& particles, float deltaTime) {
    for (auto& p : particles) {
        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;
    }
}

void CPUPhysicsEngine::handleWallCollisions(std::vector<Particle>& particles,
                                            const SimulationParameters& params,
                                            float width, float height) {
    for (auto& p : particles) {
        // Left wall
        if (p.x - p.radius < 0) {
            p.x = p.radius;
            p.vx = -p.vx * params.elasticity;
        }
        // Right wall
        if (p.x + p.radius > width) {
            p.x = width - p.radius;
            p.vx = -p.vx * params.elasticity;
        }
        // Top wall
        if (p.y - p.radius < 0) {
            p.y = p.radius;
            p.vy = -p.vy * params.elasticity;
        }
        // Bottom wall
        if (p.y + p.radius > height) {
            p.y = height - p.radius;
            p.vy = -p.vy * params.elasticity;
        }
    }
}

void CPUPhysicsEngine::handleParticleCollisions(std::vector<Particle>& particles,
                                                const SimulationParameters& params) {
    int n = static_cast<int>(particles.size());
    if (n < 2) return;

    // Clear grid
    for (auto& cell : m_grid) {
        cell.clear();
    }

    // Populate grid
    for (int i = 0; i < n; ++i) {
        int cellX = static_cast<int>(particles[i].x / m_cellSize);
        int cellY = static_cast<int>(particles[i].y / m_cellSize);
        cellX = std::clamp(cellX, 0, m_gridWidth - 1);
        cellY = std::clamp(cellY, 0, m_gridHeight - 1);
        m_grid[cellY * m_gridWidth + cellX].push_back(i);
    }

    // Check collisions using spatial grid
    for (int i = 0; i < n; ++i) {
        Particle& p1 = particles[i];
        int cellX = static_cast<int>(p1.x / m_cellSize);
        int cellY = static_cast<int>(p1.y / m_cellSize);
        cellX = std::clamp(cellX, 0, m_gridWidth - 1);
        cellY = std::clamp(cellY, 0, m_gridHeight - 1);

        // Check neighboring cells
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = cellX + dx;
                int ny = cellY + dy;
                if (nx < 0 || nx >= m_gridWidth || ny < 0 || ny >= m_gridHeight)
                    continue;

                for (int j : m_grid[ny * m_gridWidth + nx]) {
                    if (j <= i) continue;

                    Particle& p2 = particles[j];

                    float dx = p2.x - p1.x;
                    float dy = p2.y - p1.y;
                    float distSq = dx * dx + dy * dy;
                    float minDist = p1.radius + p2.radius;

                    if (distSq < minDist * minDist && distSq > 0.0001f) {
                        float dist = std::sqrt(distSq);
                        float overlap = minDist - dist;

                        // Normalize collision vector
                        float nx = dx / dist;
                        float ny = dy / dist;

                        // Separate particles
                        float totalMass = p1.mass + p2.mass;
                        float p1Ratio = p2.mass / totalMass;
                        float p2Ratio = p1.mass / totalMass;

                        p1.x -= nx * overlap * p1Ratio;
                        p1.y -= ny * overlap * p1Ratio;
                        p2.x += nx * overlap * p2Ratio;
                        p2.y += ny * overlap * p2Ratio;

                        // Calculate relative velocity
                        float relVelX = p1.vx - p2.vx;
                        float relVelY = p1.vy - p2.vy;
                        float relVelN = relVelX * nx + relVelY * ny;

                        // Only resolve if particles are approaching
                        if (relVelN > 0) {
                            // Calculate impulse
                            float impulse = (1.0f + params.elasticity) * relVelN / totalMass;

                            // Apply impulse
                            p1.vx -= impulse * p2.mass * nx;
                            p1.vy -= impulse * p2.mass * ny;
                            p2.vx += impulse * p1.mass * nx;
                            p2.vy += impulse * p1.mass * ny;

                            m_collisionCount++;
                        }
                    }
                }
            }
        }
    }
}
