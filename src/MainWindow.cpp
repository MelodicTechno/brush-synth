#include "MainWindow.h"
#include "TextureGenerator.h"
#include "AbrWriter.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QFileDialog>
#include <QMessageBox>
#include <QBuffer>
#include <QDataStream>
#include <QFile>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
    generateBrush();
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // Settings Panel
    QGroupBox* settingsGroup = new QGroupBox("Settings", this);
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsGroup);

    auto addSetting = [&](QString name, QSlider*& slider, int min, int max, int val) {
        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel(name));
        
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setValue(val);
        // Connect slider change to real-time generation
        connect(slider, &QSlider::valueChanged, this, &MainWindow::generateBrush);
        
        row->addWidget(slider);
        settingsLayout->addLayout(row);
    };

    addSetting("Canvas Size:", m_canvasSizeSlider, 64, 2048, 500);
    addSetting("Noise Count:", m_countSlider, 1, 10000, 1000);
    addSetting("Size Mean:", m_sizeMeanSlider, 1, 100, 5);
    addSetting("Size Jitter (%):", m_sizeJitterSlider, 0, 100, 50);
    addSetting("Opacity Mean:", m_opacityMeanSlider, 1, 255, 128);
    addSetting("Opacity Jitter (%):", m_opacityJitterSlider, 0, 100, 50);
    addSetting("Roundness (%):", m_roundnessSlider, 1, 100, 100);
    addSetting("Angle (deg):", m_angleSlider, 0, 360, 0);
    addSetting("Falloff (%):", m_falloffSlider, 0, 100, 0);

    // Remove Manual Generate Button if real-time is fast enough, 
    // but keeping it is fine. User asked for real-time preview logic.
    QPushButton* generateBtn = new QPushButton("Generate", this);
    connect(generateBtn, &QPushButton::clicked, this, &MainWindow::generateBrush);
    settingsLayout->addWidget(generateBtn);
    
    settingsLayout->addStretch();

    QPushButton* exportPngBtn = new QPushButton("Export PNG", this);
    connect(exportPngBtn, &QPushButton::clicked, this, &MainWindow::exportPng);
    settingsLayout->addWidget(exportPngBtn);

    QPushButton* exportAbrBtn = new QPushButton("Export ABR", this);
    connect(exportAbrBtn, &QPushButton::clicked, this, &MainWindow::exportAbr);
    settingsLayout->addWidget(exportAbrBtn);

    mainLayout->addWidget(settingsGroup, 1);

    // Preview Panel
    m_previewWidget = new PreviewWidget(this);
    mainLayout->addWidget(m_previewWidget, 3);
    
    resize(800, 600);
}

void MainWindow::generateBrush() {
    TextureGenerator::Parameters params;
    params.canvasSize = m_canvasSizeSlider->value();
    params.count = m_countSlider->value();
    params.sizeMean = m_sizeMeanSlider->value();
    params.sizeJitter = m_sizeJitterSlider->value();
    params.opacityMean = m_opacityMeanSlider->value();
    params.opacityJitter = m_opacityJitterSlider->value();
    params.roundness = m_roundnessSlider->value();
    params.angle = m_angleSlider->value();
    params.falloff = m_falloffSlider->value();

    m_brushImage = TextureGenerator::generate(params);

    m_previewWidget->setImage(m_brushImage);
}

void MainWindow::exportPng() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Brush PNG", "", "Images (*.png)");
    if (!fileName.isEmpty()) {
        m_brushImage.save(fileName);
    }
}

#include <QFileInfo>

// Simple ABR v1 Writer Implementation attempt
// Based on available reverse-engineering info
void MainWindow::exportAbr() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Brush ABR", "", "Photoshop Brush (*.abr)");
    if (fileName.isEmpty()) return;

    QFileInfo fileInfo(fileName);
    QString brushName = fileInfo.baseName();

    if (!AbrWriter::writeAbr(fileName, m_brushImage, brushName, 25)) {
        QMessageBox::warning(this, "Error", "Failed to save brush file.");
    }
}
