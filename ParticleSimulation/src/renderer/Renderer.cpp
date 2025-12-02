#include "Renderer.h"
#include "SimulatorConfig.h"
#include <raylib.h>

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::render(const std::vector<Particle>& particles) {
    // Render particles directly (no scaling - simulation space matches render space)
    for (const auto& p : particles) {
        DrawCircle(
            static_cast<int>(p.x),
            static_cast<int>(p.y),
            p.radius,
            Color{p.r, p.g, p.b, p.a}
        );
    }
}

void Renderer::clear() {
    ClearBackground(Color{m_bgR, m_bgG, m_bgB, 255});
}

void Renderer::setBackgroundColor(unsigned char r, unsigned char g, unsigned char b) {
    m_bgR = r;
    m_bgG = g;
    m_bgB = b;
}
