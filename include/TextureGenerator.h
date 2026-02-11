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
        int distributionSquareness; // 0-100

        // Shape Synthesis Params
        int shapeId; // 0=Circle, 1=Triangle, 2=Square, 3=Polygon
        int polygonSides; // 3-16
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
            
            // Apply Distribution Squareness
            if (params.distributionSquareness > 0) {
                 // Square boundary max radius at this angle
                 double absCos = std::abs(std::cos(theta));
                 double absSin = std::abs(std::sin(theta));
                 double maxR_sq = 1.0 / std::max(absCos, absSin);
                 
                 // Interpolate between Circle (1.0) and Square (maxR_sq)
                 double scaleF = 1.0 + (params.distributionSquareness / 100.0) * (maxR_sq - 1.0);
                 u *= scaleF;
                 v *= scaleF;
            }

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
                
                // Determine Polygon Properties
                double n = 0; // 0 means circle
                double rotationOffset = 0;
                
                if (params.shapeId == 1) { // Triangle
                    n = 3.0;
                    rotationOffset = M_PI / 6.0; // Rotate to point up
                } else if (params.shapeId == 2) { // Square
                    n = 4.0;
                    rotationOffset = M_PI / 4.0; // Rotate to align with axes
                } else if (params.shapeId == 3) { // Polygon
                    n = (double)params.polygonSides;
                    rotationOffset = -M_PI / 2.0; // Usually start at top
                    if (n < 3) n = 3;
                }

                for (int j = 0; j <= steps; ++j) {
                    double t = (double)j / steps * 2 * M_PI;
                    
                    // Base Shape Radius
                    double currentR = radius;
                    
                    if (n > 0) {
                         // Regular Polygon Polar Formula
                         // r(t) = R * cos(pi/n) / cos(fmod(t, 2pi/n) - pi/n)
                         // We need to apply rotation offset to t for the formula?
                         // Actually, the formula assumes vertices at specific angles.
                         // Let's adjust t for calculation
                         double an = 2 * M_PI / n;
                         double t_rot = t + rotationOffset; 
                         // Normalize to 0..2pi
                         // t_rot = std::fmod(t_rot, 2*M_PI); 
                         // if (t_rot < 0) t_rot += 2*M_PI;
                         
                         // We want fmod(t_rot, an) - an/2
                         double he = std::fmod(t_rot, an);
                         if (he < 0) he += an;
                         he -= an / 2.0;
                         
                         double scale = std::cos(M_PI / n) / std::cos(he);
                         currentR *= scale; 
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
