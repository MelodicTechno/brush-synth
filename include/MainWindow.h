#pragma once

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QGroupBox>
#include "PreviewWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void generateBrush();
    void exportPng();
    void copyToClipboard();

private:
    void setupUi();

    QImage m_brushImage;
    PreviewWidget* m_previewWidget;
    
    QSlider* m_countSlider;
    QSlider* m_sizeMeanSlider;
    QSlider* m_sizeJitterSlider;
    QSlider* m_opacityMeanSlider;
    QSlider* m_opacityJitterSlider;
    QSlider* m_roundnessSlider;
    QSlider* m_angleSlider;
    QSlider* m_falloffSlider;
    QSlider* m_distributionSquarenessSlider;
    QSlider* m_canvasSizeSlider;

    // Shape Synthesis Controls
    QComboBox* m_shapeCombo;
    QSlider* m_polygonSidesSlider;
    QSlider* m_shapeEdgeFreqSlider;
    QSlider* m_shapeEdgeAmpSlider;
    QSlider* m_shapeWarpFreqSlider;
    QSlider* m_shapeWarpAmpSlider;

    // Particle Transform Controls
    QSlider* m_particleAngleSlider;
    QSlider* m_particleAngleJitterSlider;
    QSlider* m_particleRoundnessSlider;
};
