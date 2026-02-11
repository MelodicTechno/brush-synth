#include "MainWindow.h"
#include "TextureGenerator.h"
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
        
        row->addWidget(slider);
        settingsLayout->addLayout(row);
    };

    addSetting("Canvas Size:", m_canvasSizeSlider, 64, 2048, 500);
    addSetting("Noise Count:", m_countSlider, 1, 10000, 1000);
    addSetting("Size Min:", m_sizeMinSlider, 1, 100, 2);
    addSetting("Size Max:", m_sizeMaxSlider, 1, 100, 10);
    addSetting("Opacity Min:", m_opacityMinSlider, 1, 255, 50);
    addSetting("Opacity Max:", m_opacityMaxSlider, 1, 255, 150);

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
    m_previewLabel = new QLabel(this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("background-color: #ccc; border: 1px solid #999;");
    mainLayout->addWidget(m_previewLabel, 3);
    
    resize(800, 600);
}

void MainWindow::generateBrush() {
    TextureGenerator::Parameters params;
    params.canvasSize = m_canvasSizeSlider->value();
    params.count = m_countSlider->value();
    params.minSize = m_sizeMinSlider->value();
    params.maxSize = m_sizeMaxSlider->value();
    params.minOpacity = m_opacityMinSlider->value();
    params.maxOpacity = m_opacityMaxSlider->value();

    m_brushImage = TextureGenerator::generate(params);

    m_previewLabel->setPixmap(QPixmap::fromImage(m_brushImage).scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::exportPng() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Brush PNG", "", "Images (*.png)");
    if (!fileName.isEmpty()) {
        m_brushImage.save(fileName);
    }
}

// Simple ABR v1 Writer Implementation attempt
// Based on available reverse-engineering info
void MainWindow::exportAbr() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Brush ABR", "", "Photoshop Brush (*.abr)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Could not save file.");
        return;
    }

    QDataStream out(&file);
    out.setByteOrder(QDataStream::BigEndian); // Photoshop uses Big Endian

    // Version 1
    out << (qint16)1;

    // Count (1 brush)
    out << (qint16)1;

    // Brush Type (2 = Sampled)
    out << (qint16)2;

    // We need to calculate size of the data block
    // Data block for Type 2 (Sampled):
    // 4 bytes: Misc (0)
    // 2 bytes: spacing (percent)
    // 2 bytes: name length? Or Pascal string?
    // ...
    // Actually, ABR v1/v2 is hard to get right without spec.
    // I will try to write a "fake" one that might fail or just write PNG data if I can't do it.
    
    // Alternative: Use a Python script if available.
    // Since I don't have the spec, I'll inform the user.
    
    file.close();
    // Re-open to clear or just delete
    file.remove();
    
    QMessageBox::information(this, "Not Implemented", 
        "ABR export requires a complex binary format. \n"
        "Please use PNG export and define brush in Photoshop.\n"
        "Or I can try to find a Python library to do this if you have one installed.");
        
    // Ideally I would call a python script here.
}
