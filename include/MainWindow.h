#pragma once

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include "PreviewWidget.h"

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

    QImage m_brushImage;
    PreviewWidget* m_previewWidget;
    
    QSlider* m_countSlider;
    QSlider* m_sizeMinSlider;
    QSlider* m_sizeMaxSlider;
    QSlider* m_opacityMinSlider;
    QSlider* m_opacityMaxSlider;
    QSlider* m_roundnessSlider;
    QSlider* m_angleSlider;
    QSlider* m_falloffSlider;
    QSlider* m_canvasSizeSlider;
};
