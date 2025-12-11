/**
 * @file Application.cpp
 * @brief Classe principale de l'application - Orchestre tous les composants
 *
 * ARCHITECTURE HYBRIDE Qt6 + CUDA + Raylib :
 *
 * ┌──────────────────────────────────────────────┐
 * │           Application (ce fichier)           │
 * │  - Initialise tous les composants            │
 * │  - Gère la boucle d'événements Qt            │
 * │  - Coordonne UI ↔ Physics ↔ Renderer         │
 * └──────────────────────────────────────────────┘
 *          ↓              ↓              ↓
 *    ┌─────────┐   ┌──────────┐   ┌──────────┐
 *    │  Qt UI  │   │ Physics  │   │  Raylib  │
 *    │ (Panel) │   │  Engine  │   │ (Render) │
 *    └─────────┘   └──────────┘   └──────────┘
 *
 * RESPONSABILITÉS :
 * - Détection automatique de CUDA (GPU disponible?)
 * - Sélection dynamique CPU/GPU selon le mode choisi
 * - Gestion du cycle de vie des composants
 */

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

/**
 * @brief Point d'entrée principal de l'application
 *
 * Initialise Qt, crée les composants, et démarre la boucle d'événements.
 *
 * @return Code de retour Qt (0 si succès)
 */
int Application::run() {
    QApplication qtApp(m_argc, m_argv);
    qtApp.setApplicationName("Particle Simulator");

    // Initialise tous les composants (UI, Physics, Renderer)
    initializeComponents();

    // Affiche la fenêtre principale
    m_mainWindow->show();

    // Démarre la boucle d'événements Qt (bloquant jusqu'à fermeture)
    return qtApp.exec();
}

/**
 * @brief Initialise tous les composants de l'application
 *
 * ORDRE D'INITIALISATION :
 * 1. SimulationState (données partagées)
 * 2. CPUPhysicsEngine (toujours disponible)
 * 3. GPUPhysicsEngine (si CUDA compilé et disponible)
 * 4. Sélection automatique GPU/CPU selon disponibilité
 * 5. Renderer (Raylib)
 * 6. MainWindow (Qt UI + intégration Raylib)
 */
void Application::initializeComponents() {
    // Crée l'état de simulation (partagé entre tous les composants)
    m_state = std::make_unique<SimulationState>();

    // Crée le moteur physique CPU (fallback, toujours disponible)
    m_cpuEngine = std::make_unique<CPUPhysicsEngine>();

#ifdef USE_CUDA
    // Crée le moteur physique GPU (si compilé avec CUDA)
    m_gpuEngine = std::make_unique<GPUPhysicsEngine>();

    // Sélectionne automatiquement le GPU si disponible
    if (m_gpuEngine->isAvailable()) {
        m_currentEngine = m_gpuEngine.get();
        m_state->getParameters().useGPU = true;
    } else {
        // GPU non disponible (pas de CUDA Toolkit ou GPU NVIDIA)
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
