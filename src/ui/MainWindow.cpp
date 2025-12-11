#include "MainWindow.h"
#include "ControlPanel.h"
#include "renderer/RaylibWindow.h"
#include "core/Application.h"
#include "core/SimulationState.h"
#include "SimulatorConfig.h"

#include <QHBoxLayout>
#include <QCloseEvent>

MainWindow::MainWindow(Application* app, QWidget* parent)
    : QMainWindow(parent), m_app(app) {

    setupUI();

    setWindowTitle("Simulateur de Particules Hybride - Qt6 + CUDA + Raylib");
    setFixedSize(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(centralWidget);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create Raylib render window
    m_raylibWindow = new RaylibWindow(m_app, this);

    // Create control panel
    m_controlPanel = new ControlPanel(m_app, this);

    // Add to layout
    layout->addWidget(m_raylibWindow);
    layout->addWidget(m_controlPanel);

    setCentralWidget(centralWidget);

    // Connect signals
    connect(m_controlPanel, &ControlPanel::startClicked,
            this, [this]() {
                m_app->onStart();
                m_raylibWindow->startSimulation();
            });

    connect(m_controlPanel, &ControlPanel::pauseClicked,
            m_app, &Application::onPause);

    connect(m_controlPanel, &ControlPanel::resetClicked,
            m_app, &Application::onReset);

    connect(m_controlPanel, &ControlPanel::computeModeChanged,
            m_app, &Application::onComputeModeChanged);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    m_raylibWindow->stopSimulation();
    m_app->getState()->setRunning(false);
    event->accept();
}
