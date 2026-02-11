#pragma once

#include <QImage>
#include <QPainter>
#include <QRandomGenerator>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class TextureGenerator {
public:
    struct Parameters {
        int canvasSize;
        int count;
        int sizeMean;      // Average size
        int sizeJitter;    // 0-100 (Variance percentage)
        int opacityMean;   // Average opacity
        int opacityJitter; // 0-100 (Variance percentage)
        int roundness; // 0-100
        int angle;     // 0-360
        int falloff;   // 0-100
    };

    static QImage generate(const Parameters& params) {
        QImage image(params.canvasSize, params.canvasSize, QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);

        auto* rng = QRandomGenerator::global();
        double centerX = params.canvasSize / 2.0;
        double centerY = params.canvasSize / 2.0;
        double maxRadius = params.canvasSize / 2.0;

        double angleRad = params.angle * M_PI / 180.0;
        double cosA = std::cos(angleRad);
        double sinA = std::sin(angleRad);
        double roundnessFactor = params.roundness / 100.0;
        if (roundnessFactor < 0.01) roundnessFactor = 0.01;

        for (int i = 0; i < params.count; ++i) {
            // Size calculation with Mean and Jitter
            // Jitter is percentage deviation. E.g. 50 mean, 10 jitter -> range [45, 55] (approx)
            // Let's implement uniform distribution: [mean * (1 - jitter/100), mean * (1 + jitter/100)]
            double sizeVar = params.sizeMean * (params.sizeJitter / 100.0);
            int s = std::round(params.sizeMean + rng->generateDouble() * 2.0 * sizeVar - sizeVar);
            if (s < 1) s = 1;

            // Opacity calculation with Mean and Jitter
            double opacityVar = params.opacityMean * (params.opacityJitter / 100.0);
            int alpha = std::round(params.opacityMean + rng->generateDouble() * 2.0 * opacityVar - opacityVar);
            if (alpha < 0) alpha = 0;
            if (alpha > 255) alpha = 255;

            // Polar coordinates generation
            double rRand = rng->generateDouble(); // 0..1
            
            // Adjust distribution based on falloff
            // If falloff is 0 (uniform), r ~ sqrt(u) to account for area increase
            // If falloff is high, we want r to be small.
            // Let's mix:
            // Uniform: r = sqrt(u)
            // Concentrated: r = u^power
            
            double r;
            if (params.falloff == 0) {
                 r = std::sqrt(rRand);
            } else {
                 // params.falloff 1..100
                 // Map to power 1.0 .. 5.0?
                 double p = 1.0 + (params.falloff / 20.0); 
                 // But wait, if we want Gaussian-like, we need more control.
                 // Simple approach: r = pow(rRand, p) where p > 1 biases towards 0?
                 // Wait, rRand is 0..1. 0 is center.
                 // If p=2, 0.5^2 = 0.25. 0.9^2 = 0.81.
                 // Yes, higher power pulls values towards 0.
                 r = std::pow(rRand, p);
            }

            r *= maxRadius;

            double theta = rng->generateDouble() * 2 * M_PI;

            // Raw circle coordinates
            double u = r * std::cos(theta);
            double v = r * std::sin(theta);

            // Apply roundness (squeeze Y)
            v *= roundnessFactor;

            // Apply rotation
            double xRot = u * cosA - v * sinA;
            double yRot = u * sinA + v * cosA;

            // Translate to center
            double finalX = centerX + xRot;
            double finalY = centerY + yRot;

            QColor color(0, 0, 0, alpha);
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(finalX, finalY), s / 2.0, s / 2.0);
        }

        return image;
    }
};
