#pragma once

#include <memory>
#include <QObject>

class SimulationState;
class PhysicsEngine;
class CPUPhysicsEngine;
#ifdef USE_CUDA
class GPUPhysicsEngine;
#endif
class Renderer;
class MainWindow;

class Application : public QObject {
    Q_OBJECT

public:
    Application(int argc, char* argv[]);
    ~Application();

    int run();

    SimulationState* getState();
    PhysicsEngine* getPhysicsEngine();
    Renderer* getRenderer();

public slots:
    void onStart();
    void onPause();
    void onReset();
    void onParametersChanged();
    void onComputeModeChanged(bool useGPU);

private:
    void initializeComponents();
    void switchPhysicsEngine(bool useGPU);

    std::unique_ptr<SimulationState> m_state;
    std::unique_ptr<CPUPhysicsEngine> m_cpuEngine;
#ifdef USE_CUDA
    std::unique_ptr<GPUPhysicsEngine> m_gpuEngine;
#endif
    PhysicsEngine* m_currentEngine = nullptr;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<MainWindow> m_mainWindow;

    int m_argc;
    char** m_argv;
};
