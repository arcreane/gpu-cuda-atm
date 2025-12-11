#include "ControlPanel.h"
#include "core/Application.h"
#include "core/SimulationState.h"
#include "SimulatorConfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

ControlPanel::ControlPanel(Application* app, QWidget* parent)
    : QWidget(parent), m_app(app), m_state(app->getState()) {

    setupUI();

    // Stats update timer
    m_statsTimer = new QTimer(this);
    connect(m_statsTimer, &QTimer::timeout, this, &ControlPanel::updateStats);
    m_statsTimer->start(100); // Update every 100ms
}

ControlPanel::~ControlPanel() = default;

void ControlPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    mainLayout->addWidget(createInitGroup());
    mainLayout->addWidget(createPhysicsGroup());
    mainLayout->addWidget(createMouseGroup());
    mainLayout->addWidget(createControlGroup());
    mainLayout->addWidget(createStatsGroup());
    mainLayout->addStretch();

    setFixedWidth(Config::UI_WIDTH);
}

QGroupBox* ControlPanel::createInitGroup() {
    QGroupBox* group = new QGroupBox("Initialisation", this);
    QFormLayout* layout = new QFormLayout(group);

    // Particle count
    m_particleCountSpin = new QSpinBox(this);
    m_particleCountSpin->setRange(Config::MIN_PARTICLES, Config::MAX_PARTICLES);
    m_particleCountSpin->setValue(m_state->getParameters().particleCount);
    m_particleCountSpin->setSingleStep(100);
    connect(m_particleCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ControlPanel::onParticleCountChanged);
    layout->addRow("Nombre de particules:", m_particleCountSpin);

    // Min radius
    QHBoxLayout* minRadiusLayout = new QHBoxLayout();
    m_minRadiusSlider = new QSlider(Qt::Horizontal, this);
    m_minRadiusSlider->setRange(20, 150);
    m_minRadiusSlider->setValue(static_cast<int>(m_state->getParameters().minRadius * 10));
    m_minRadiusLabel = new QLabel(QString::number(m_state->getParameters().minRadius, 'f', 1), this);
    connect(m_minRadiusSlider, &QSlider::valueChanged, this, &ControlPanel::onMinRadiusChanged);
    minRadiusLayout->addWidget(m_minRadiusSlider);
    minRadiusLayout->addWidget(m_minRadiusLabel);
    layout->addRow("Rayon min:", minRadiusLayout);

    // Max radius
    QHBoxLayout* maxRadiusLayout = new QHBoxLayout();
    m_maxRadiusSlider = new QSlider(Qt::Horizontal, this);
    m_maxRadiusSlider->setRange(20, 150);
    m_maxRadiusSlider->setValue(static_cast<int>(m_state->getParameters().maxRadius * 10));
    m_maxRadiusLabel = new QLabel(QString::number(m_state->getParameters().maxRadius, 'f', 1), this);
    connect(m_maxRadiusSlider, &QSlider::valueChanged, this, &ControlPanel::onMaxRadiusChanged);
    maxRadiusLayout->addWidget(m_maxRadiusSlider);
    maxRadiusLayout->addWidget(m_maxRadiusLabel);
    layout->addRow("Rayon max:", maxRadiusLayout);

    return group;
}

QGroupBox* ControlPanel::createPhysicsGroup() {
    QGroupBox* group = new QGroupBox("Physique", this);
    QFormLayout* layout = new QFormLayout(group);

    // Min velocity
    QHBoxLayout* minVelLayout = new QHBoxLayout();
    m_minVelocitySlider = new QSlider(Qt::Horizontal, this);
    m_minVelocitySlider->setRange(0, 2000);
    m_minVelocitySlider->setValue(static_cast<int>(m_state->getParameters().minVelocity * 10));
    m_minVelocityLabel = new QLabel(QString::number(m_state->getParameters().minVelocity, 'f', 1), this);
    connect(m_minVelocitySlider, &QSlider::valueChanged, this, &ControlPanel::onMinVelocityChanged);
    minVelLayout->addWidget(m_minVelocitySlider);
    minVelLayout->addWidget(m_minVelocityLabel);
    layout->addRow("Vitesse min:", minVelLayout);

    // Max velocity
    QHBoxLayout* maxVelLayout = new QHBoxLayout();
    m_maxVelocitySlider = new QSlider(Qt::Horizontal, this);
    m_maxVelocitySlider->setRange(0, 2000);
    m_maxVelocitySlider->setValue(static_cast<int>(m_state->getParameters().maxVelocity * 10));
    m_maxVelocityLabel = new QLabel(QString::number(m_state->getParameters().maxVelocity, 'f', 1), this);
    connect(m_maxVelocitySlider, &QSlider::valueChanged, this, &ControlPanel::onMaxVelocityChanged);
    maxVelLayout->addWidget(m_maxVelocitySlider);
    maxVelLayout->addWidget(m_maxVelocityLabel);
    layout->addRow("Vitesse max:", maxVelLayout);

    // Elasticity
    QHBoxLayout* elasticityLayout = new QHBoxLayout();
    m_elasticitySlider = new QSlider(Qt::Horizontal, this);
    m_elasticitySlider->setRange(0, 100);
    m_elasticitySlider->setValue(static_cast<int>(m_state->getParameters().elasticity * 100));
    m_elasticityLabel = new QLabel(QString::number(m_state->getParameters().elasticity, 'f', 2), this);
    connect(m_elasticitySlider, &QSlider::valueChanged, this, &ControlPanel::onElasticityChanged);
    elasticityLayout->addWidget(m_elasticitySlider);
    elasticityLayout->addWidget(m_elasticityLabel);
    layout->addRow("Elasticite:", elasticityLayout);

    // Friction
    QHBoxLayout* frictionLayout = new QHBoxLayout();
    m_frictionSlider = new QSlider(Qt::Horizontal, this);
    m_frictionSlider->setRange(0, 100);
    m_frictionSlider->setValue(static_cast<int>(m_state->getParameters().friction * 100));
    m_frictionLabel = new QLabel(QString::number(m_state->getParameters().friction, 'f', 2), this);
    connect(m_frictionSlider, &QSlider::valueChanged, this, &ControlPanel::onFrictionChanged);
    frictionLayout->addWidget(m_frictionSlider);
    frictionLayout->addWidget(m_frictionLabel);
    layout->addRow("Frottement:", frictionLayout);

    // Gravity
    QHBoxLayout* gravityLayout = new QHBoxLayout();
    m_gravitySlider = new QSlider(Qt::Horizontal, this);
    m_gravitySlider->setRange(0, 1000);
    m_gravitySlider->setValue(static_cast<int>(m_state->getParameters().gravity));
    m_gravityLabel = new QLabel(QString::number(m_state->getParameters().gravity, 'f', 0), this);
    connect(m_gravitySlider, &QSlider::valueChanged, this, &ControlPanel::onGravityChanged);
    gravityLayout->addWidget(m_gravitySlider);
    gravityLayout->addWidget(m_gravityLabel);
    layout->addRow("Gravite:", gravityLayout);

    return group;
}

QGroupBox* ControlPanel::createMouseGroup() {
    QGroupBox* group = new QGroupBox("Interaction Souris", this);
    QFormLayout* layout = new QFormLayout(group);

    // Mouse force radius
    QHBoxLayout* radiusLayout = new QHBoxLayout();
    m_mouseRadiusSlider = new QSlider(Qt::Horizontal, this);
    m_mouseRadiusSlider->setRange(10, 500);
    m_mouseRadiusSlider->setValue(static_cast<int>(m_state->getParameters().mouseForceRadius));
    m_mouseRadiusLabel = new QLabel(QString::number(m_state->getParameters().mouseForceRadius, 'f', 0), this);
    connect(m_mouseRadiusSlider, &QSlider::valueChanged, this, &ControlPanel::onMouseRadiusChanged);
    radiusLayout->addWidget(m_mouseRadiusSlider);
    radiusLayout->addWidget(m_mouseRadiusLabel);
    layout->addRow("Rayon souris:", radiusLayout);

    // Mouse force strength
    QHBoxLayout* strengthLayout = new QHBoxLayout();
    m_mouseStrengthSlider = new QSlider(Qt::Horizontal, this);
    m_mouseStrengthSlider->setRange(100, 20000);
    m_mouseStrengthSlider->setValue(static_cast<int>(m_state->getParameters().mouseForceStrength));
    m_mouseStrengthLabel = new QLabel(QString::number(m_state->getParameters().mouseForceStrength, 'f', 0), this);
    connect(m_mouseStrengthSlider, &QSlider::valueChanged, this, &ControlPanel::onMouseStrengthChanged);
    strengthLayout->addWidget(m_mouseStrengthSlider);
    strengthLayout->addWidget(m_mouseStrengthLabel);
    layout->addRow("Force souris:", strengthLayout);

    return group;
}

QGroupBox* ControlPanel::createControlGroup() {
    QGroupBox* group = new QGroupBox("Controle", this);
    QVBoxLayout* layout = new QVBoxLayout(group);

    // Compute mode
    QHBoxLayout* modeLayout = new QHBoxLayout();
    QLabel* modeLabel = new QLabel("Mode:", this);
    m_computeModeCombo = new QComboBox(this);
    m_computeModeCombo->addItem("CPU (Sequentiel)");
#ifdef USE_CUDA
    m_computeModeCombo->addItem("GPU (CUDA)");
    m_computeModeCombo->setCurrentIndex(m_state->getParameters().useGPU ? 1 : 0);
#else
    m_computeModeCombo->setCurrentIndex(0);
    m_computeModeCombo->setEnabled(false); // Disable when CUDA is not available
#endif
    connect(m_computeModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ControlPanel::onComputeModeChanged);
    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(m_computeModeCombo);
    layout->addLayout(modeLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton("Demarrer", this);
    m_pauseButton = new QPushButton("Pause", this);
    m_resetButton = new QPushButton("Reset", this);

    connect(m_startButton, &QPushButton::clicked, this, &ControlPanel::onStartClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &ControlPanel::onPauseClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &ControlPanel::onResetClicked);

    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_resetButton);
    layout->addLayout(buttonLayout);

    return group;
}

QGroupBox* ControlPanel::createStatsGroup() {
    QGroupBox* group = new QGroupBox("Statistiques", this);
    QFormLayout* layout = new QFormLayout(group);

    m_fpsLabel = new QLabel("0", this);
    m_particlesLabel = new QLabel("0", this);
    m_physicsTimeLabel = new QLabel("0 ms", this);
    m_renderTimeLabel = new QLabel("0 ms", this);
    m_collisionsLabel = new QLabel("0", this);

    layout->addRow("FPS:", m_fpsLabel);
    layout->addRow("Particules:", m_particlesLabel);
    layout->addRow("Physique:", m_physicsTimeLabel);
    layout->addRow("Rendu:", m_renderTimeLabel);
    layout->addRow("Collisions/frame:", m_collisionsLabel);

    return group;
}

void ControlPanel::onParticleCountChanged(int value) {
    m_state->getParameters().particleCount = value;
}

void ControlPanel::onMinRadiusChanged(int value) {
    float radius = value / 10.0f;
    m_state->getParameters().minRadius = radius;
    m_minRadiusLabel->setText(QString::number(radius, 'f', 1));

    // Ensure max >= min
    if (m_maxRadiusSlider->value() < value) {
        m_maxRadiusSlider->setValue(value);
    }
}

void ControlPanel::onMaxRadiusChanged(int value) {
    float radius = value / 10.0f;
    m_state->getParameters().maxRadius = radius;
    m_maxRadiusLabel->setText(QString::number(radius, 'f', 1));

    // Ensure min <= max
    if (m_minRadiusSlider->value() > value) {
        m_minRadiusSlider->setValue(value);
    }
}

void ControlPanel::onMinVelocityChanged(int value) {
    float velocity = value / 10.0f;
    m_state->getParameters().minVelocity = velocity;
    m_minVelocityLabel->setText(QString::number(velocity, 'f', 1));

    if (m_maxVelocitySlider->value() < value) {
        m_maxVelocitySlider->setValue(value);
    }
}

void ControlPanel::onMaxVelocityChanged(int value) {
    float velocity = value / 10.0f;
    m_state->getParameters().maxVelocity = velocity;
    m_maxVelocityLabel->setText(QString::number(velocity, 'f', 1));

    if (m_minVelocitySlider->value() > value) {
        m_minVelocitySlider->setValue(value);
    }
}

void ControlPanel::onElasticityChanged(int value) {
    float elasticity = value / 100.0f;
    m_state->getParameters().elasticity = elasticity;
    m_elasticityLabel->setText(QString::number(elasticity, 'f', 2));
}

void ControlPanel::onFrictionChanged(int value) {
    float friction = value / 100.0f;
    m_state->getParameters().friction = friction;
    m_frictionLabel->setText(QString::number(friction, 'f', 2));
}

void ControlPanel::onGravityChanged(int value) {
    m_state->getParameters().gravity = static_cast<float>(value);
    m_gravityLabel->setText(QString::number(value));
}

void ControlPanel::onMouseRadiusChanged(int value) {
    m_state->getParameters().mouseForceRadius = static_cast<float>(value);
    m_mouseRadiusLabel->setText(QString::number(value));
}

void ControlPanel::onMouseStrengthChanged(int value) {
    m_state->getParameters().mouseForceStrength = static_cast<float>(value);
    m_mouseStrengthLabel->setText(QString::number(value));
}

void ControlPanel::onComputeModeChanged(int index) {
    bool useGPU = (index == 1);
    emit computeModeChanged(useGPU);
}

void ControlPanel::onStartClicked() {
    emit startClicked();
    m_startButton->setEnabled(false);
    m_pauseButton->setEnabled(true);
}

void ControlPanel::onPauseClicked() {
    emit pauseClicked();
    if (m_state->isPaused()) {
        m_pauseButton->setText("Reprendre");
    } else {
        m_pauseButton->setText("Pause");
    }
}

void ControlPanel::onResetClicked() {
    emit resetClicked();
    m_startButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    m_pauseButton->setText("Pause");
}

void ControlPanel::updateStats() {
    const PerformanceStats& stats = m_state->getStats();
    m_fpsLabel->setText(QString::number(static_cast<int>(stats.fps)));
    m_particlesLabel->setText(QString::number(m_state->getParticleCount()));
    m_physicsTimeLabel->setText(QString::number(stats.physicsTimeMs, 'f', 2) + " ms");
    m_renderTimeLabel->setText(QString::number(stats.renderTimeMs, 'f', 2) + " ms");
    m_collisionsLabel->setText(QString::number(stats.collisionCount));
}
