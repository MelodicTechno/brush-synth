#pragma once

#include <QImage>
#include <QPainter>
#include <QPainterPath>
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

        // Shape Synthesis Params
        int shapeId; // 0=Circle, 1=Square, 2=Triangle
        int shapeEdgeFreq; // 0-50
        int shapeEdgeAmp; // 0-100 (Percentage of radius)

        // Particle Transform Params
        int particleAngle; // 0-360
        int particleAngleJitter; // 0-100%
        int particleRoundness; // 1-100%
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
            // Size calculation
            double sizeVar = params.sizeMean * (params.sizeJitter / 100.0);
            int s = std::round(params.sizeMean + rng->generateDouble() * 2.0 * sizeVar - sizeVar);
            if (s < 1) s = 1;
            double radius = s / 2.0;

            // Opacity calculation
            double opacityVar = params.opacityMean * (params.opacityJitter / 100.0);
            int alpha = std::round(params.opacityMean + rng->generateDouble() * 2.0 * opacityVar - opacityVar);
            if (alpha < 0) alpha = 0;
            if (alpha > 255) alpha = 255;

            // Distribution logic
            double rRand = rng->generateDouble(); 
            double r;
            if (params.falloff == 0) {
                 r = std::sqrt(rRand);
            } else {
                 double p = 1.0 + (params.falloff / 20.0); 
                 r = std::pow(rRand, p);
            }
            r *= maxRadius;
            double theta = rng->generateDouble() * 2 * M_PI;

            double u = r * std::cos(theta);
            double v = r * std::sin(theta);
            v *= roundnessFactor;
            double xRot = u * cosA - v * sinA;
            double yRot = u * sinA + v * cosA;
            double finalX = centerX + xRot;
            double finalY = centerY + yRot;

            QColor color(0, 0, 0, alpha);
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);

            // Apply Particle Transform
            painter.save();
            painter.translate(finalX, finalY);

            // Rotation
            double pAngle = params.particleAngle;
            if (params.particleAngleJitter > 0) {
                 double jitterRange = 360.0 * (params.particleAngleJitter / 100.0);
                 pAngle += (rng->generateDouble() - 0.5) * jitterRange; 
            }
            painter.rotate(pAngle);

            // Roundness (Scale Y)
            double pRound = params.particleRoundness / 100.0;
            if (pRound < 0.01) pRound = 0.01;
            painter.scale(1.0, pRound); 

            // Shape Generation (Draw at 0,0)
            if (params.shapeId == 0 && params.shapeEdgeFreq == 0) {
                // Optimization for simple circle
                painter.drawEllipse(QPointF(0, 0), radius, radius);
            } else {
                QPainterPath path;
                int steps = 30 + std::min(s, 100); // Dynamic resolution
                if (params.shapeEdgeFreq > 0) steps = std::max(steps, params.shapeEdgeFreq * 4); // Increase resolution for high freq
                
                for (int j = 0; j <= steps; ++j) {
                    double t = (double)j / steps * 2 * M_PI;
                    
                    // Base Shape Radius
                    double currentR = radius;
                    
                    // Shape Type Modulation
                    if (params.shapeId == 1) { // Square
                        // Polar equation for square: r = 1 / max(|cos|, |sin|)
                        // Rotate by 45 deg (PI/4) to align square
                        double t_sq = t - M_PI / 4.0;
                        double scale = std::max(std::abs(std::cos(t_sq)), std::abs(std::sin(t_sq)));
                        if (scale > 0.001) currentR /= scale;
                        currentR *= 0.8; 
                    } else if (params.shapeId == 2) { // Triangle
                        double n = 3.0;
                        double an = 2 * M_PI / n;
                        double t_off = t + M_PI / 6.0; // Rotate to point up
                        double he = std::fmod(t_off, an) - an / 2.0;
                        double scale = std::cos(M_PI / n) / std::cos(he);
                        currentR /= scale; 
                        currentR *= 0.6; 
                    }

                    // Edge Modulation (FM Synthesis equivalent)
                    if (params.shapeEdgeFreq > 0 && params.shapeEdgeAmp > 0) {
                        double wave = std::sin(t * params.shapeEdgeFreq);
                        double phase = i * 13.5; 
                        wave = std::sin(t * params.shapeEdgeFreq + phase);
                        
                        double ampFactor = params.shapeEdgeAmp / 100.0;
                        currentR *= (1.0 + wave * ampFactor);
                    }

                    double px = currentR * std::cos(t);
                    double py = currentR * std::sin(t);
                    
                    if (j == 0) path.moveTo(px, py);
                    else path.lineTo(px, py);
                }
                path.closeSubpath();
                
                // No translation needed, we are at 0,0 local space
                painter.drawPath(path);
            }
            painter.restore();
        }

        return image;
    }
};
