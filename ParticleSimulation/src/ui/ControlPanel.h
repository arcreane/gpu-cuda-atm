#pragma once

#include <QWidget>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QTimer>

class Application;
class SimulationState;

class ControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit ControlPanel(Application* app, QWidget* parent = nullptr);
    ~ControlPanel() override;

signals:
    void startClicked();
    void pauseClicked();
    void resetClicked();
    void computeModeChanged(bool useGPU);

private slots:
    void onParticleCountChanged(int value);
    void onMinRadiusChanged(int value);
    void onMaxRadiusChanged(int value);
    void onMinVelocityChanged(int value);
    void onMaxVelocityChanged(int value);
    void onElasticityChanged(int value);
    void onFrictionChanged(int value);
    void onGravityChanged(int value);
    void onMouseRadiusChanged(int value);
    void onMouseStrengthChanged(int value);
    void onComputeModeChanged(int index);
    void onStartClicked();
    void onPauseClicked();
    void onResetClicked();
    void updateStats();

private:
    void setupUI();
    QGroupBox* createInitGroup();
    QGroupBox* createPhysicsGroup();
    QGroupBox* createMouseGroup();
    QGroupBox* createControlGroup();
    QGroupBox* createStatsGroup();

    Application* m_app;
    SimulationState* m_state;

    // Initialization
    QSpinBox* m_particleCountSpin;

    // Particle size
    QSlider* m_minRadiusSlider;
    QSlider* m_maxRadiusSlider;
    QLabel* m_minRadiusLabel;
    QLabel* m_maxRadiusLabel;

    // Velocity
    QSlider* m_minVelocitySlider;
    QSlider* m_maxVelocitySlider;
    QLabel* m_minVelocityLabel;
    QLabel* m_maxVelocityLabel;

    // Physics
    QSlider* m_elasticitySlider;
    QSlider* m_frictionSlider;
    QSlider* m_gravitySlider;
    QLabel* m_elasticityLabel;
    QLabel* m_frictionLabel;
    QLabel* m_gravityLabel;

    // Mouse interaction
    QSlider* m_mouseRadiusSlider;
    QSlider* m_mouseStrengthSlider;
    QLabel* m_mouseRadiusLabel;
    QLabel* m_mouseStrengthLabel;

    // Control
    QComboBox* m_computeModeCombo;
    QPushButton* m_startButton;
    QPushButton* m_pauseButton;
    QPushButton* m_resetButton;

    // Stats
    QLabel* m_fpsLabel;
    QLabel* m_particlesLabel;
    QLabel* m_physicsTimeLabel;
    QLabel* m_renderTimeLabel;
    QLabel* m_collisionsLabel;

    QTimer* m_statsTimer;
};
