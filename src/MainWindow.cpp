#include "MainWindow.h"
#include "TextureGenerator.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QFileDialog>
#include <QMessageBox>
#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QClipboard>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
    m_isInitializing = false;
    generateBrush();
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // Settings Panel
    QGroupBox* settingsGroup = new QGroupBox("Settings", this);
    settingsGroup->setMinimumWidth(320); // Ensure panel is wide enough
    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsGroup);

    auto addSetting = [&](QString name, QSlider*& slider, int min, int max, int val) {
        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel(name));
        
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setValue(val);
        slider->setMinimumWidth(100); // Ensure slider doesn't collapse to a dot
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
    m_shapeCombo->addItem("Wavetable", 4);
    shapeTypeRow->addWidget(m_shapeCombo);
    shapeLayout->addLayout(shapeTypeRow);

    // Helper for rows
    auto addShapeRow = [&](QString text, QSlider*& slider, QLabel*& labelOut, QWidget*& rowOut, int min, int max, int val) {
        rowOut = new QWidget();
        QHBoxLayout* l = new QHBoxLayout(rowOut);
        l->setContentsMargins(0,0,0,0);
        labelOut = new QLabel(text);
        l->addWidget(labelOut);
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setValue(val);
        slider->setMinimumWidth(100);
        connect(slider, &QSlider::valueChanged, this, &MainWindow::generateBrush);
        l->addWidget(slider);
        shapeLayout->addWidget(rowOut);
    };

    // Polygon Sides (Hidden by default unless Polygon)
    QLabel* polyLabel;
    addShapeRow("Polygon Sides (3-16):", m_polygonSidesSlider, polyLabel, m_polygonSidesRow, 3, 16, 5);

    // Wave Threshold (Hidden by default unless Wavetable)
    QLabel* threshLabel;
    addShapeRow("Wavetable Threshold:", m_waveThresholdSlider, threshLabel, m_waveThresholdRow, 0, 100, 50);

    // Standard Params (Always visible, labels change)
    QWidget* dummyRow;
    addShapeRow("Edge Frequency (FM Freq):", m_shapeEdgeFreqSlider, m_shapeEdgeFreqLabel, dummyRow, 0, 50, 0);
    addShapeRow("Edge Amplitude (FM Depth %):", m_shapeEdgeAmpSlider, m_shapeEdgeAmpLabel, dummyRow, 0, 100, 0);
    addShapeRow("Phase Warp Freq (Twist):", m_shapeWarpFreqSlider, m_shapeWarpFreqLabel, dummyRow, 1, 20, 1);
    addShapeRow("Phase Warp Amp (Twist Strength):", m_shapeWarpAmpSlider, m_shapeWarpAmpLabel, dummyRow, 0, 100, 0);

    // UI Update Logic
    connect(m_shapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](){
        int idx = m_shapeCombo->currentData().toInt();
        bool isPoly = (idx == 3);
        bool isWave = (idx == 4);
        
        m_polygonSidesRow->setVisible(isPoly);
        m_waveThresholdRow->setVisible(isWave);
        
        if (isWave) {
             m_shapeEdgeFreqLabel->setText("Freq X:");
             m_shapeEdgeAmpLabel->setText("FM Amount:");
             m_shapeWarpFreqLabel->setText("Freq Y:");
             m_shapeWarpAmpLabel->setText("Phase Y:");
        } else {
             m_shapeEdgeFreqLabel->setText("Edge Frequency (FM Freq):");
             m_shapeEdgeAmpLabel->setText("Edge Amplitude (FM Depth %):");
             m_shapeWarpFreqLabel->setText("Phase Warp Freq (Twist):");
             m_shapeWarpAmpLabel->setText("Phase Warp Amp (Twist Strength):");
        }
        generateBrush();
    });
    
    // Trigger initial state
    emit m_shapeCombo->currentIndexChanged(0);

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
        slider->setMinimumWidth(100);
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

    QPushButton* copyClipboardBtn = new QPushButton("Copy to Clipboard", this);
    connect(copyClipboardBtn, &QPushButton::clicked, this, &MainWindow::copyToClipboard);
    settingsLayout->addWidget(copyClipboardBtn);

    mainLayout->addWidget(settingsGroup, 1);

    // Preview Panel
    m_previewWidget = new PreviewWidget(this);
    mainLayout->addWidget(m_previewWidget, 3);
    
    resize(800, 600);
}

void MainWindow::generateBrush() {
    if (m_isInitializing) return;

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
    params.shapeWarpFreq = m_shapeWarpFreqSlider->value();
    params.shapeWarpAmp = m_shapeWarpAmpSlider->value();
    params.waveThreshold = m_waveThresholdSlider->value();

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

void MainWindow::copyToClipboard() {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(m_brushImage);
    // Optional: Feedback to user
    // QMessageBox::information(this, "Copied", "Brush image copied to clipboard.");
}
