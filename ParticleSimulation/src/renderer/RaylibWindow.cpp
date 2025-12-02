#include "RaylibWindow.h"
#include "Renderer.h"
#include "core/Application.h"
#include "core/SimulationState.h"
#include "physics/PhysicsEngine.h"
#include "SimulatorConfig.h"

#include <QResizeEvent>
#include <chrono>

#ifdef _WIN32
// Rename Windows functions that conflict with raylib BEFORE including windows.h
#define CloseWindow CloseWindow_Win32
#define ShowCursor ShowCursor_Win32
#define DrawText DrawText_Win32
#define DrawTextEx DrawTextEx_Win32
#define Rectangle Rectangle_Win32
#define LoadImage LoadImage_Win32
#define PlaySound PlaySound_Win32

#include <windows.h>

// Now undefine them so raylib can use the original names
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx
#undef Rectangle
#undef LoadImage
#undef PlaySound
#endif

#include <raylib.h>

RaylibWindow::RaylibWindow(Application* app, QWidget* parent)
    : QWidget(parent), m_app(app) {

    // Set fixed size for render area
    setFixedSize(Config::RENDER_WIDTH, Config::RENDER_HEIGHT);

    // Important: This allows Raylib to render to this widget
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NativeWindow);
    setMouseTracking(true);

    // Create update timer
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &RaylibWindow::updateSimulation);
}

RaylibWindow::~RaylibWindow() {
    cleanupRaylib();
}

void RaylibWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!m_raylibInitialized) {
        initRaylib();
    }
}

void RaylibWindow::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    m_timer->stop();
}

void RaylibWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

void RaylibWindow::initRaylib() {
    if (m_raylibInitialized) return;

#ifdef _WIN32
    // Get the native window handle
    HWND hwnd = (HWND)winId();

    // Get actual widget size accounting for DPI scaling
    qreal dpr = devicePixelRatio();
    int physicalWidth = static_cast<int>(width() * dpr);
    int physicalHeight = static_cast<int>(height() * dpr);

    // Set Raylib to use this window
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(physicalWidth, physicalHeight, "");

    // Get Raylib's window and set it as child of Qt widget
    HWND raylibHwnd = GetActiveWindow();
    SetParent(raylibHwnd, hwnd);
    SetWindowLong(raylibHwnd, GWL_STYLE, WS_CHILD | WS_VISIBLE);
    SetWindowPos(raylibHwnd, HWND_TOP, 0, 0,
                 physicalWidth, physicalHeight,
                 SWP_FRAMECHANGED);
#else
    // For other platforms, create a regular window
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(Config::RENDER_WIDTH, Config::RENDER_HEIGHT, "Particle Simulator");
#endif

    SetTargetFPS(60);
    m_raylibInitialized = true;
}

void RaylibWindow::cleanupRaylib() {
    if (m_raylibInitialized) {
        CloseWindow();
        m_raylibInitialized = false;
    }
}

void RaylibWindow::startSimulation() {
    if (!m_raylibInitialized) {
        initRaylib();
    }
    m_timer->start(16); // ~60 FPS
}

void RaylibWindow::stopSimulation() {
    m_timer->stop();
}

void RaylibWindow::handleMouseInput() {
    SimulationState* state = m_app->getState();
    MouseState& mouse = state->getMouseState();

    // Get mouse position relative to widget (in logical pixels)
    QPoint pos = mapFromGlobal(QCursor::pos());

    // Convert from logical pixels to physical pixels (simulation space)
    qreal dpr = devicePixelRatio();
    float physicalX = static_cast<float>(pos.x() * dpr);
    float physicalY = static_cast<float>(pos.y() * dpr);

    // Calculate mouse velocity (in physical pixels)
    float vx = physicalX - m_lastMouseX;
    float vy = physicalY - m_lastMouseY;

    // Check if mouse is in render area (logical pixel check)
    bool inArea = (pos.x() >= 0 && pos.x() < Config::WINDOW_RENDER_WIDTH &&
                   pos.y() >= 0 && pos.y() < Config::WINDOW_RENDER_HEIGHT);

    // Get button states from Raylib
    mouse.leftButtonDown = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    mouse.rightButtonDown = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

    state->updateMouseState(physicalX, physicalY, vx, vy, inArea);

    m_lastMouseX = physicalX;
    m_lastMouseY = physicalY;
}

void RaylibWindow::updateSimulation() {
    if (!m_raylibInitialized) return;

    SimulationState* state = m_app->getState();
    if (!state->isRunning()) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Handle mouse input
    handleMouseInput();

    // Update physics if not paused
    if (!state->isPaused()) {
        PhysicsEngine* engine = m_app->getPhysicsEngine();
        if (engine) {
            std::lock_guard<std::mutex> lock(state->getMutex());
            engine->update(state->getParticles(),
                         state->getParameters(),
                         state->getMouseState(),
                         Config::FIXED_DELTA_TIME);
            state->getStats().collisionCount = engine->getCollisionCount();
        }
    }

    auto physicsEnd = std::chrono::high_resolution_clock::now();

    // Render
    BeginDrawing();
    m_app->getRenderer()->clear();

    {
        std::lock_guard<std::mutex> lock(state->getMutex());
        m_app->getRenderer()->render(state->getParticles());
    }

    EndDrawing();

    auto renderEnd = std::chrono::high_resolution_clock::now();

    // Update stats
    PerformanceStats& stats = state->getStats();
    stats.fps = static_cast<float>(GetFPS());
    stats.physicsTimeMs = std::chrono::duration<float, std::milli>(physicsEnd - startTime).count();
    stats.renderTimeMs = std::chrono::duration<float, std::milli>(renderEnd - physicsEnd).count();
}
