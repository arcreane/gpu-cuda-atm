#pragma once

#include <vector>
#include "core/SimulationState.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    void render(const std::vector<Particle>& particles);
    void clear();

    void setBackgroundColor(unsigned char r, unsigned char g, unsigned char b);

private:
    unsigned char m_bgR = 20;
    unsigned char m_bgG = 20;
    unsigned char m_bgB = 30;
};
