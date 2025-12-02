#include "SimulationState.h"
#include "SimulatorConfig.h"
#include <random>
#include <cmath>

SimulationState::SimulationState() {
    m_parameters.particleCount = Config::DEFAULT_PARTICLE_COUNT;
    m_parameters.minRadius = Config::DEFAULT_MIN_RADIUS;
    m_parameters.maxRadius = Config::DEFAULT_MAX_RADIUS;
    m_parameters.elasticity = Config::DEFAULT_ELASTICITY;
    m_parameters.friction = Config::DEFAULT_FRICTION;
    m_parameters.gravity = Config::DEFAULT_GRAVITY;
}

SimulationState::~SimulationState() = default;

void SimulationState::initializeParticles(int count) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_particles.clear();
    m_particles.reserve(count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posX(50.0f, Config::SIMULATION_WIDTH - 50.0f);
    std::uniform_real_distribution<float> posY(50.0f, Config::SIMULATION_HEIGHT - 50.0f);
    std::uniform_real_distribution<float> radius(m_parameters.minRadius, m_parameters.maxRadius);
    std::uniform_real_distribution<float> velocity(-m_parameters.maxVelocity, m_parameters.maxVelocity);
    std::uniform_int_distribution<int> color(100, 255);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.radius = radius(gen);
        p.x = posX(gen);
        p.y = posY(gen);

        // Ensure particles start within bounds
        p.x = std::max(p.radius, std::min(p.x, Config::SIMULATION_WIDTH - p.radius));
        p.y = std::max(p.radius, std::min(p.y, Config::SIMULATION_HEIGHT - p.radius));

        float angle = std::uniform_real_distribution<float>(0.0f, 2.0f * 3.14159f)(gen);
        float speed = std::uniform_real_distribution<float>(m_parameters.minVelocity, m_parameters.maxVelocity)(gen);
        p.vx = speed * std::cos(angle);
        p.vy = speed * std::sin(angle);

        p.mass = p.radius * p.radius; // Mass proportional to area

        // Color based on size
        float t = (p.radius - m_parameters.minRadius) / (m_parameters.maxRadius - m_parameters.minRadius + 0.001f);
        p.r = static_cast<unsigned char>(50 + 150 * (1.0f - t));
        p.g = static_cast<unsigned char>(100 + 100 * t);
        p.b = static_cast<unsigned char>(200 + 55 * t);
        p.a = 255;

        m_particles.push_back(p);
    }

    m_parameters.particleCount = count;
}

void SimulationState::resetParticles() {
    initializeParticles(m_parameters.particleCount);
}

std::vector<Particle>& SimulationState::getParticles() {
    return m_particles;
}

const std::vector<Particle>& SimulationState::getParticles() const {
    return m_particles;
}

int SimulationState::getParticleCount() const {
    return static_cast<int>(m_particles.size());
}

SimulationParameters& SimulationState::getParameters() {
    return m_parameters;
}

const SimulationParameters& SimulationState::getParameters() const {
    return m_parameters;
}

void SimulationState::setParameters(const SimulationParameters& params) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_parameters = params;
}

MouseState& SimulationState::getMouseState() {
    return m_mouseState;
}

const MouseState& SimulationState::getMouseState() const {
    return m_mouseState;
}

void SimulationState::updateMouseState(float x, float y, float vx, float vy, bool inArea) {
    m_mouseState.x = x;
    m_mouseState.y = y;
    m_mouseState.vx = vx;
    m_mouseState.vy = vy;
    m_mouseState.isInRenderArea = inArea;
}

PerformanceStats& SimulationState::getStats() {
    return m_stats;
}

const PerformanceStats& SimulationState::getStats() const {
    return m_stats;
}

bool SimulationState::isRunning() const {
    return m_running.load();
}

void SimulationState::setRunning(bool running) {
    m_running.store(running);
}

bool SimulationState::isPaused() const {
    return m_paused.load();
}

void SimulationState::setPaused(bool paused) {
    m_paused.store(paused);
}

std::mutex& SimulationState::getMutex() {
    return m_mutex;
}
