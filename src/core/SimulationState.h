#pragma once

#include <vector>
#include <atomic>
#include <mutex>

struct Particle {
    float x, y;           // Position
    float vx, vy;         // Velocity
    float radius;         // Radius
    float mass;           // Mass
    unsigned char r, g, b, a; // Color
};

struct SimulationParameters {
    int particleCount = 1000;
    float minRadius = 3.0f;
    float maxRadius = 8.0f;
    float minVelocity = 0.0f;
    float maxVelocity = 100.0f;
    float elasticity = 0.8f;
    float friction = 0.02f;
    float gravity = 200.0f;
    bool useGPU = true;
    float mouseForceRadius = 100.0f;
    float mouseForceStrength = 5000.0f;
};

struct MouseState {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    bool isInRenderArea = false;
    bool leftButtonDown = false;
    bool rightButtonDown = false;
};

struct PerformanceStats {
    float fps = 0.0f;
    float physicsTimeMs = 0.0f;
    float renderTimeMs = 0.0f;
    int collisionCount = 0;
};

class SimulationState {
public:
    SimulationState();
    ~SimulationState();

    // Particle management
    void initializeParticles(int count);
    void resetParticles();
    std::vector<Particle>& getParticles();
    const std::vector<Particle>& getParticles() const;
    int getParticleCount() const;

    // Parameters
    SimulationParameters& getParameters();
    const SimulationParameters& getParameters() const;
    void setParameters(const SimulationParameters& params);

    // Mouse state
    MouseState& getMouseState();
    const MouseState& getMouseState() const;
    void updateMouseState(float x, float y, float vx, float vy, bool inArea);

    // Performance
    PerformanceStats& getStats();
    const PerformanceStats& getStats() const;

    // Simulation control
    bool isRunning() const;
    void setRunning(bool running);
    bool isPaused() const;
    void setPaused(bool paused);

    // Thread safety
    std::mutex& getMutex();

private:
    std::vector<Particle> m_particles;
    SimulationParameters m_parameters;
    MouseState m_mouseState;
    PerformanceStats m_stats;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::mutex m_mutex;
};
