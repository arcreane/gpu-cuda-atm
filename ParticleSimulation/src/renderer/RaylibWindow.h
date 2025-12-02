#pragma once

#include <QWidget>
#include <QTimer>
#include <memory>

class SimulationState;
class PhysicsEngine;
class Renderer;
class Application;

class RaylibWindow : public QWidget {
    Q_OBJECT

public:
    explicit RaylibWindow(Application* app, QWidget* parent = nullptr);
    ~RaylibWindow() override;

    void startSimulation();
    void stopSimulation();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    QPaintEngine* paintEngine() const override { return nullptr; }

private slots:
    void updateSimulation();

private:
    void initRaylib();
    void cleanupRaylib();
    void handleMouseInput();

    Application* m_app;
    QTimer* m_timer;

    bool m_raylibInitialized = false;
    float m_lastMouseX = 0;
    float m_lastMouseY = 0;
};
