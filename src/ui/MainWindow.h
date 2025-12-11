#pragma once

#include <QMainWindow>

class Application;
class ControlPanel;
class RaylibWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Application* app, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUI();

    Application* m_app;
    ControlPanel* m_controlPanel;
    RaylibWindow* m_raylibWindow;
};
