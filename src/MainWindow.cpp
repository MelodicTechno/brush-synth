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
    addSetting("Distribution Squareness:", m_distributionSquarenessSlider, 0, 100, 0);

    // Shape Synthesis UI
    QGroupBox* shapeGroup = new QGroupBox("Shape Synthesis", this);
    QVBoxLayout* shapeLayout = new QVBoxLayout(shapeGroup);
    
    // Shape Type Combo
    QHBoxLayout* shapeTypeRow = new QHBoxLayout();
    shapeTypeRow->addWidget(new QLabel("Shape Type:"));
    m_shapeCombo = new QComboBox();
    m_shapeCombo->addItem("Circle", 0);
    m_shapeCombo->addItem("Triangle", 1);
    m_shapeCombo->addItem("Square", 2);
    m_shapeCombo->addItem("Polygon", 3);
    connect(m_shapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::generateBrush);
    shapeTypeRow->addWidget(m_shapeCombo);
    shapeLayout->addLayout(shapeTypeRow);

    auto addShapeSetting = [&](QString name, QSlider*& slider, int min, int max, int val) {
        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel(name));
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setValue(val);
        connect(slider, &QSlider::valueChanged, this, &MainWindow::generateBrush);
        row->addWidget(slider);
        shapeLayout->addLayout(row);
    };

    addShapeSetting("Polygon Sides (3-16):", m_polygonSidesSlider, 3, 16, 5);
    addShapeSetting("Edge Frequency (FM Freq):", m_shapeEdgeFreqSlider, 0, 50, 0);
    addShapeSetting("Edge Amplitude (FM Depth %):", m_shapeEdgeAmpSlider, 0, 100, 0);

    settingsLayout->addWidget(shapeGroup);

    // Particle Transform UI
    QGroupBox* particleTransformGroup = new QGroupBox("Particle Transform", this);
    QVBoxLayout* ptLayout = new QVBoxLayout(particleTransformGroup);

    auto addPtSetting = [&](QString name, QSlider*& slider, int min, int max, int val) {
        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel(name));
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setValue(val);
        connect(slider, &QSlider::valueChanged, this, &MainWindow::generateBrush);
        row->addWidget(slider);
        ptLayout->addLayout(row);
    };

    addPtSetting("Particle Angle (deg):", m_particleAngleSlider, 0, 360, 0);
    addPtSetting("Angle Jitter (%):", m_particleAngleJitterSlider, 0, 100, 0);
    addPtSetting("Roundness (Stretch %):", m_particleRoundnessSlider, 1, 100, 100);

    settingsLayout->addWidget(particleTransformGroup);

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
    params.distributionSquareness = m_distributionSquarenessSlider->value();
    
    params.shapeId = m_shapeCombo->currentIndex();
    params.polygonSides = m_polygonSidesSlider->value();
    params.shapeEdgeFreq = m_shapeEdgeFreqSlider->value();
    params.shapeEdgeAmp = m_shapeEdgeAmpSlider->value();

    params.particleAngle = m_particleAngleSlider->value();
    params.particleAngleJitter = m_particleAngleJitterSlider->value();
    params.particleRoundness = m_particleRoundnessSlider->value();

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
