#include "Application.h"
#include "SimulationState.h"
#include "physics/CPUPhysicsEngine.h"
#ifdef USE_CUDA
#include "physics/GPUPhysicsEngine.h"
#endif
#include "renderer/Renderer.h"
#include "ui/MainWindow.h"
#include "SimulatorConfig.h"

#include <QApplication>
#include <QTimer>

Application::Application(int argc, char* argv[])
    : m_argc(argc), m_argv(argv) {
}

Application::~Application() = default;

int Application::run() {
    QApplication qtApp(m_argc, m_argv);
    qtApp.setApplicationName("Particle Simulator");

    initializeComponents();

    m_mainWindow->show();

    return qtApp.exec();
}

void Application::initializeComponents() {
    // Create simulation state
    m_state = std::make_unique<SimulationState>();

    // Create physics engines
    m_cpuEngine = std::make_unique<CPUPhysicsEngine>();

#ifdef USE_CUDA
    m_gpuEngine = std::make_unique<GPUPhysicsEngine>();

    // Default to GPU if available
    if (m_gpuEngine->isAvailable()) {
        m_currentEngine = m_gpuEngine.get();
        m_state->getParameters().useGPU = true;
    } else {
        m_currentEngine = m_cpuEngine.get();
        m_state->getParameters().useGPU = false;
    }
#else
    // Use CPU engine when CUDA is not available
    m_currentEngine = m_cpuEngine.get();
    m_state->getParameters().useGPU = false;
#endif

    // Create renderer
    m_renderer = std::make_unique<Renderer>();

    // Create main window
    m_mainWindow = std::make_unique<MainWindow>(this);

    // Initialize particles
    m_state->initializeParticles(m_state->getParameters().particleCount);

    // Initialize physics engine with particles
    m_currentEngine->initialize(m_state->getParticles());
}

void Application::switchPhysicsEngine(bool useGPU) {
#ifdef USE_CUDA
    PhysicsEngine* newEngine = useGPU ?
        static_cast<PhysicsEngine*>(m_gpuEngine.get()) :
        static_cast<PhysicsEngine*>(m_cpuEngine.get());

    if (newEngine != m_currentEngine) {
        m_currentEngine = newEngine;
        m_currentEngine->initialize(m_state->getParticles());
    }
#else
    // When CUDA is not available, always use CPU engine
    m_currentEngine = m_cpuEngine.get();
    m_state->getParameters().useGPU = false;
#endif
}

SimulationState* Application::getState() {
    return m_state.get();
}

PhysicsEngine* Application::getPhysicsEngine() {
    return m_currentEngine;
}

Renderer* Application::getRenderer() {
    return m_renderer.get();
}

void Application::onStart() {
    m_state->setRunning(true);
    m_state->setPaused(false);
}

void Application::onPause() {
    m_state->setPaused(!m_state->isPaused());
}

void Application::onReset() {
    m_state->setPaused(true);
    m_state->resetParticles();
    m_currentEngine->initialize(m_state->getParticles());
}

void Application::onParametersChanged() {
    // Parameters are updated in real-time through the state
}

void Application::onComputeModeChanged(bool useGPU) {
#ifdef USE_CUDA
    m_state->getParameters().useGPU = useGPU;
    switchPhysicsEngine(useGPU);
#else
    // Force CPU when CUDA is not available
    m_state->getParameters().useGPU = false;
#endif
}
