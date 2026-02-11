#pragma once

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void generateBrush();
    void exportAbr();
    void exportPng();

private:
    void setupUi();
    void drawNoise();

    QImage m_brushImage;
    QLabel* m_previewLabel;
    
    QSlider* m_countSlider;
    QSlider* m_sizeMinSlider;
    QSlider* m_sizeMaxSlider;
    QSlider* m_opacityMinSlider;
    QSlider* m_opacityMaxSlider;
    QSlider* m_canvasSizeSlider;
};
