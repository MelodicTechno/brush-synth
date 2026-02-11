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
    
    QSpinBox* m_countSpin;
    QSpinBox* m_sizeMinSpin;
    QSpinBox* m_sizeMaxSpin;
    QSpinBox* m_opacityMinSpin;
    QSpinBox* m_opacityMaxSpin;
    QSpinBox* m_canvasSizeSpin;
};
