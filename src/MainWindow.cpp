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
#include <QMimeData>

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
    
    // Distribution Controls
    QHBoxLayout* distTypeRow = new QHBoxLayout();
    distTypeRow->addWidget(new QLabel("Distribution:"));
    m_distTypeCombo = new QComboBox();
    m_distTypeCombo->addItem("Random", 0);
    m_distTypeCombo->addItem("Grid", 1);
    m_distTypeCombo->addItem("Spiral", 2);
    connect(m_distTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::generateBrush);
    distTypeRow->addWidget(m_distTypeCombo);
    settingsLayout->addLayout(distTypeRow);

    addSetting("Dist Jitter/Spread:", m_distJitterSlider, 0, 100, 0);

    // Global Controls
    addSetting("Roundness (Scale Y):", m_roundnessSlider, 1, 100, 100);
    addSetting("Angle:", m_angleSlider, 0, 360, 0);
    addSetting("Squareness (Boundary):", m_distributionSquarenessSlider, 0, 100, 0);
    addSetting("Falloff (Density):", m_falloffSlider, 0, 100, 0); // 0 = uniform, 100 = strong center bias

    // Shape Group
    QGroupBox* shapeGroup = new QGroupBox("Shape Synthesis", this);
    QVBoxLayout* shapeLayout = new QVBoxLayout(shapeGroup);
    
    QHBoxLayout* shapeTypeRow = new QHBoxLayout();
    shapeTypeRow->addWidget(new QLabel("Shape Type:"));
    m_shapeCombo = new QComboBox();
    m_shapeCombo->addItem("Circle", 0);
    m_shapeCombo->addItem("Square", 1);
    m_shapeCombo->addItem("Triangle", 2);
    m_shapeCombo->addItem("Polygon", 3);
    m_shapeCombo->addItem("Wavetable", 4);
    connect(m_shapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::generateBrush);
    shapeTypeRow->addWidget(m_shapeCombo);
    shapeLayout->addLayout(shapeTypeRow);
    
    auto addShapeRow = [&](QString name, QSlider*& slider, QLabel*& labelPtr, QWidget*& rowWidgetPtr, int min, int max, int val) {
        QWidget* rowWidget = new QWidget();
        QHBoxLayout* row = new QHBoxLayout(rowWidget);
        row->setContentsMargins(0,0,0,0);
        labelPtr = new QLabel(name);
        row->addWidget(labelPtr);
        
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setValue(val);
        connect(slider, &QSlider::valueChanged, this, &MainWindow::generateBrush);
        row->addWidget(slider);
        
        shapeLayout->addWidget(rowWidget);
        rowWidgetPtr = rowWidget;
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

    // Create Tab Widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setMinimumWidth(340);
    
    // Tab 1: Generator
    QWidget* generatorTab = new QWidget();
    QVBoxLayout* genLayout = new QVBoxLayout(generatorTab);
    genLayout->setContentsMargins(5,5,5,5);
    genLayout->addWidget(settingsGroup);
    m_tabWidget->addTab(generatorTab, "Generator");
    
    // Tab 2: Presets
    QWidget* presetsTab = new QWidget();
    QVBoxLayout* presetsLayout = new QVBoxLayout(presetsTab);
    
    presetsLayout->addWidget(new QLabel("Saved Presets:"));
    m_presetList = new QListWidget();
    connect(m_presetList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*){
        loadPreset();
    });
    presetsLayout->addWidget(m_presetList);
    
    QHBoxLayout* nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel("Name:"));
    m_presetNameEdit = new QLineEdit();
    nameRow->addWidget(m_presetNameEdit);
    presetsLayout->addLayout(nameRow);
    
    QHBoxLayout* btnRow = new QHBoxLayout();
    m_savePresetButton = new QPushButton("Save");
    m_loadPresetButton = new QPushButton("Load");
    m_deletePresetButton = new QPushButton("Delete");
    
    connect(m_savePresetButton, &QPushButton::clicked, this, &MainWindow::savePreset);
    connect(m_loadPresetButton, &QPushButton::clicked, this, &MainWindow::loadPreset);
    connect(m_deletePresetButton, &QPushButton::clicked, this, &MainWindow::deletePreset);
    
    btnRow->addWidget(m_savePresetButton);
    btnRow->addWidget(m_loadPresetButton);
    btnRow->addWidget(m_deletePresetButton);
    presetsLayout->addLayout(btnRow);
    
    m_refreshPresetsButton = new QPushButton("Refresh List");
    connect(m_refreshPresetsButton, &QPushButton::clicked, this, &MainWindow::refreshPresets);
    presetsLayout->addWidget(m_refreshPresetsButton);
    
    presetsLayout->addStretch();
    m_tabWidget->addTab(presetsTab, "Presets");
    
    // Initial refresh
    refreshPresets();

    mainLayout->addWidget(m_tabWidget, 1);

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
    params.distType = m_distTypeCombo->currentData().toInt();
    params.distJitter = m_distJitterSlider->value();

    params.shapeId = m_shapeCombo->currentData().toInt();
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
    QString fileName = QFileDialog::getSaveFileName(this, "Export PNG", "", "PNG Files (*.png)");
    if (!fileName.isEmpty()) {
        m_brushImage.save(fileName);
        QMessageBox::information(this, "Success", "Brush exported successfully!");
    }
}

void MainWindow::copyToClipboard() {
    QClipboard *clipboard = QApplication::clipboard();
    QMimeData *mimeData = new QMimeData;
    
    // Set standard image data (bitmap)
    mimeData->setImageData(m_brushImage);
    
    // Also set PNG format for better transparency support in modern apps
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    m_brushImage.save(&buffer, "PNG");
    mimeData->setData("image/png", byteArray);
    
    clipboard->setMimeData(mimeData);
}

QJsonObject MainWindow::serializeSettings() {
    QJsonObject json;
    json["canvasSize"] = m_canvasSizeSlider->value();
    json["count"] = m_countSlider->value();
    json["sizeMean"] = m_sizeMeanSlider->value();
    json["sizeJitter"] = m_sizeJitterSlider->value();
    json["opacityMean"] = m_opacityMeanSlider->value();
    json["opacityJitter"] = m_opacityJitterSlider->value();
    json["roundness"] = m_roundnessSlider->value();
    json["angle"] = m_angleSlider->value();
    json["falloff"] = m_falloffSlider->value();
    json["distSquareness"] = m_distributionSquarenessSlider->value();
    json["distType"] = m_distTypeCombo->currentData().toInt();
    json["distJitter"] = m_distJitterSlider->value();
    
    json["shapeId"] = m_shapeCombo->currentData().toInt();
    json["polygonSides"] = m_polygonSidesSlider->value();
    json["edgeFreq"] = m_shapeEdgeFreqSlider->value();
    json["edgeAmp"] = m_shapeEdgeAmpSlider->value();
    json["warpFreq"] = m_shapeWarpFreqSlider->value();
    json["warpAmp"] = m_shapeWarpAmpSlider->value();
    json["waveThreshold"] = m_waveThresholdSlider->value();
    
    json["particleAngle"] = m_particleAngleSlider->value();
    json["particleAngleJitter"] = m_particleAngleJitterSlider->value();
    json["particleRoundness"] = m_particleRoundnessSlider->value();
    
    return json;
}

void MainWindow::deserializeSettings(const QJsonObject& json) {
    m_isInitializing = true; // Prevent update spam
    
    if (json.contains("canvasSize")) m_canvasSizeSlider->setValue(json["canvasSize"].toInt());
    if (json.contains("count")) m_countSlider->setValue(json["count"].toInt());
    if (json.contains("sizeMean")) m_sizeMeanSlider->setValue(json["sizeMean"].toInt());
    if (json.contains("sizeJitter")) m_sizeJitterSlider->setValue(json["sizeJitter"].toInt());
    if (json.contains("opacityMean")) m_opacityMeanSlider->setValue(json["opacityMean"].toInt());
    if (json.contains("opacityJitter")) m_opacityJitterSlider->setValue(json["opacityJitter"].toInt());
    if (json.contains("roundness")) m_roundnessSlider->setValue(json["roundness"].toInt());
    if (json.contains("angle")) m_angleSlider->setValue(json["angle"].toInt());
    if (json.contains("falloff")) m_falloffSlider->setValue(json["falloff"].toInt());
    if (json.contains("distSquareness")) m_distributionSquarenessSlider->setValue(json["distSquareness"].toInt());
    
    if (json.contains("distType")) {
        int index = m_distTypeCombo->findData(json["distType"].toInt());
        if (index != -1) m_distTypeCombo->setCurrentIndex(index);
    }
    if (json.contains("distJitter")) m_distJitterSlider->setValue(json["distJitter"].toInt());
    
    if (json.contains("shapeId")) {
        int index = m_shapeCombo->findData(json["shapeId"].toInt());
        if (index != -1) m_shapeCombo->setCurrentIndex(index);
    }
    if (json.contains("polygonSides")) m_polygonSidesSlider->setValue(json["polygonSides"].toInt());
    if (json.contains("edgeFreq")) m_shapeEdgeFreqSlider->setValue(json["edgeFreq"].toInt());
    if (json.contains("edgeAmp")) m_shapeEdgeAmpSlider->setValue(json["edgeAmp"].toInt());
    if (json.contains("warpFreq")) m_shapeWarpFreqSlider->setValue(json["warpFreq"].toInt());
    if (json.contains("warpAmp")) m_shapeWarpAmpSlider->setValue(json["warpAmp"].toInt());
    if (json.contains("waveThreshold")) m_waveThresholdSlider->setValue(json["waveThreshold"].toInt());
    
    if (json.contains("particleAngle")) m_particleAngleSlider->setValue(json["particleAngle"].toInt());
    if (json.contains("particleAngleJitter")) m_particleAngleJitterSlider->setValue(json["particleAngleJitter"].toInt());
    if (json.contains("particleRoundness")) m_particleRoundnessSlider->setValue(json["particleRoundness"].toInt());

    m_isInitializing = false;
    generateBrush();
}

void MainWindow::savePreset() {
    QString name = m_presetNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Error", "Preset name cannot be empty.");
        return;
    }
    
    QDir dir("presets");
    if (!dir.exists()) dir.mkpath(".");
    
    QFile file(dir.filePath(name + ".json"));
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Cannot save preset file.");
        return;
    }
    
    QJsonObject json = serializeSettings();
    QJsonDocument doc(json);
    file.write(doc.toJson());
    file.close();
    
    refreshPresets();
}

void MainWindow::loadPreset() {
    QListWidgetItem* item = m_presetList->currentItem();
    if (!item) return;
    
    QString name = item->text();
    QFile file("presets/" + name + ".json");
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    deserializeSettings(doc.object());
    
    m_presetNameEdit->setText(name);
}

void MainWindow::deletePreset() {
    QListWidgetItem* item = m_presetList->currentItem();
    if (!item) return;
    
    QString name = item->text();
    if (QMessageBox::question(this, "Confirm", "Delete preset '" + name + "'?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        QFile::remove("presets/" + name + ".json");
        refreshPresets();
    }
}

void MainWindow::refreshPresets() {
    m_presetList->clear();
    QDir dir("presets");
    if (!dir.exists()) return;
    
    QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString& f : files) {
        m_presetList->addItem(QFileInfo(f).baseName());
    }
}
