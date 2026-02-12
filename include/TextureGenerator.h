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
        int distType; // 0=Random, 1=Grid, 2=Spiral
        int distJitter; // 0-100

        // Shape Synthesis Params
        int shapeId; // 0=Circle, 1=Triangle, 2=Square, 3=Polygon
        int polygonSides; // 3-16
        int shapeEdgeFreq; // 0-50
        int shapeEdgeAmp; // 0-100 (Percentage of radius)
        
        // Shape Distortion Params (Phase Warp)
        int shapeWarpFreq; // 1-20
        int shapeWarpAmp;  // 0-100 (Phase shift intensity)

        // Wavetable Params
        int waveThreshold; // 0-100 (Cutoff level)

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
        
        // Calculate max particle radius to avoid clipping
        double sizeVar = params.sizeMean * (params.sizeJitter / 100.0);
        double maxSize = params.sizeMean + sizeVar;
        double maxParticleRadius = maxSize / 2.0;
        
        // Account for Edge Modulation (Amplitude)
        if (params.shapeEdgeAmp > 0) {
            maxParticleRadius *= (1.0 + params.shapeEdgeAmp / 100.0);
        }
        
        // Ensure we don't reduce radius to negative
        double margin = maxParticleRadius + 2.0; // +2 for safety
        double maxRadius = (params.canvasSize / 2.0) - margin;
        if (maxRadius < 1.0) maxRadius = 1.0;

        // Pre-generate Wavetable Particle if needed
        QImage wavetableImage;
        if (params.shapeId == 4) {
            int size = std::ceil(maxSize);
            if (size < 1) size = 1;
            wavetableImage = QImage(size, size, QImage::Format_ARGB32);
            wavetableImage.fill(Qt::transparent);

            // Wavetable Generation (FM Synthesis Style)
            // Z = sin(u * fx + FM * sin(v * fy + phase))
            // Cutoff at threshold
            
            double freqX = std::max(1.0, (double)params.shapeEdgeFreq);
            double freqY = std::max(1.0, (double)params.shapeWarpFreq); // Modulator Freq
            double fmAmount = params.shapeEdgeAmp / 20.0; // FM Index
            double phaseY = params.shapeWarpAmp / 100.0 * 2.0 * M_PI;
            double threshold = (params.waveThreshold / 50.0) - 1.0; // Map 0..100 to -1..1
            
            // Invert threshold logic: User likely wants "Amount of shape", so 100% means full square, 0% means nothing.
            // Or "Threshold" means Cutoff Level.
            // User said "Keep high parts". So High Threshold = Less pixels.
            // Let's stick to "Threshold". 0 = Keep Everything (Full Square), 100 = Keep Nothing (Peaks only).
            // So T = map(val, 0, 100, -1.0, 1.0).
            
            for (int y = 0; y < size; ++y) {
                QRgb* scanLine = (QRgb*)wavetableImage.scanLine(y);
                double v = (double)y / size * 2.0 * M_PI - M_PI; // -PI to PI
                
                for (int x = 0; x < size; ++x) {
                    double u = (double)x / size * 2.0 * M_PI - M_PI; // -PI to PI
                    
                    // FM Formula
                    // Modulator
                    double mod = std::sin(v * freqY + phaseY);
                    
                    // Carrier
                    double signal = std::sin(u * freqX + fmAmount * mod);
                    
                    // To make it more interesting, let's mix X and Y?
                    // The above is strictly horizontal waves modulated vertically.
                    // If we want "Cells", we need interference.
                    // Let's add a vertical carrier component too?
                    // signal = (sin(...) + sin(v * freqY)) / 2
                    
                    // User asked for "Wavetable Cross Section".
                    // Let's try: Z = sin(u*fx) * sin(v*fy). This makes Grid.
                    // With FM: Z = sin(u*fx + mod) * sin(v*fy + mod) ?
                    
                    // Let's go with the "Interference" model (Sum):
                    // Z = (sin(u * fx + fm * mod) + sin(v * fy + phaseY)) / 2.0
                    
                    double z = (std::sin(u * freqX + fmAmount * mod) + std::sin(v * freqY + phaseY)) / 2.0;
                    
                    if (z > threshold) {
                        // Anti-aliasing
                        double edge = std::min(1.0, (z - threshold) * 10.0); // Soft edge
                        int alpha = (int)(edge * 255);
                        scanLine[x] = qRgba(0, 0, 0, alpha);
                    } else {
                        scanLine[x] = qRgba(0, 0, 0, 0);
                    }
                }
            }
        }

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

            // Distribution Logic
            double u, v; // Normalized coords
            double r_norm, theta;

            if (params.distType == 1) { // Grid
                int side = std::ceil(std::sqrt(params.count));
                if (side < 1) side = 1;
                int row = i / side;
                int col = i % side;
                
                // Normalized -1..1
                u = (side > 1) ? ((double)col / (side - 1) * 2.0 - 1.0) : 0;
                v = (side > 1) ? ((double)row / (side - 1) * 2.0 - 1.0) : 0;
                
                if (params.distJitter > 0) {
                    double cell = 2.0 / side;
                    u += (rng->generateDouble() - 0.5) * cell * (params.distJitter / 50.0);
                    v += (rng->generateDouble() - 0.5) * cell * (params.distJitter / 50.0);
                }
                
                r_norm = std::sqrt(u*u + v*v);
                theta = std::atan2(v, u);
            } else if (params.distType == 2) { // Spiral (Phyllotaxis)
                double angle = i * 2.3999632; 
                r_norm = std::sqrt((double)i / params.count);
                
                u = r_norm * std::cos(angle);
                v = r_norm * std::sin(angle);
                theta = angle;
                
                if (params.distJitter > 0) {
                    double jitterScale = 0.1; 
                    u += (rng->generateDouble() - 0.5) * jitterScale * (params.distJitter / 50.0);
                    v += (rng->generateDouble() - 0.5) * jitterScale * (params.distJitter / 50.0);
                    r_norm = std::sqrt(u*u + v*v);
                    theta = std::atan2(v, u);
                }
            } else { // Random (Default)
                r_norm = std::sqrt(rng->generateDouble()); // Uniform area
                theta = rng->generateDouble() * 2 * M_PI;
                u = r_norm * std::cos(theta);
                v = r_norm * std::sin(theta);
            }

            // 1. Falloff (Radial Warp) - Apply to all
            if (params.falloff > 0) {
                double p = 1.0 + (params.falloff / 20.0); 
                double new_r = std::pow(r_norm, p);
                if (r_norm > 1e-6) {
                    double scale = new_r / r_norm;
                    u *= scale;
                    v *= scale;
                    r_norm = new_r;
                }
            }

            // 2. Squareness (Boundary Constraint)
            // 0 = Circle, 100 = Square
            if (params.distType == 1) { // Grid: Masking
                if (params.distributionSquareness < 100) {
                     double absCos = std::abs(std::cos(theta));
                     double absSin = std::abs(std::sin(theta));
                     double maxR_sq = (absCos > absSin) ? (1.0/absCos) : (1.0/absSin);
                     if (std::isinf(maxR_sq)) maxR_sq = 1.0;

                     // Interpolate boundary: 0->1.0, 100->maxR_sq
                     double limit = 1.0 + (params.distributionSquareness / 100.0) * (maxR_sq - 1.0);
                     
                     if (r_norm > limit) continue; // Discard point
                }
            } else { // Random/Spiral: Stretching
                 if (params.distributionSquareness > 0) {
                     double absCos = std::abs(std::cos(theta));
                     double absSin = std::abs(std::sin(theta));
                     double maxR_sq = (absCos > absSin) ? (1.0/absCos) : (1.0/absSin);
                     if (std::isinf(maxR_sq)) maxR_sq = 1.0;
                     
                     double scaleF = 1.0 + (params.distributionSquareness / 100.0) * (maxR_sq - 1.0);
                     u *= scaleF;
                     v *= scaleF;
                 }
            }

            u *= maxRadius;
            v *= maxRadius;

            v *= roundnessFactor;
            double xRot = u * cosA - v * sinA;
            double yRot = u * sinA + v * cosA;
            double finalX = centerX + xRot;
            double finalY = centerY + yRot;

            // Use painter opacity to control transparency for both Shapes and Images
            painter.setOpacity(alpha / 255.0);
            QColor color(0, 0, 0, 255); 
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
            if (params.shapeId == 4) {
                 // Wavetable Image
                 // Draw centered at 0,0
                 double offset = s / 2.0;
                 // Draw the pre-generated image scaled to current particle size
                 painter.drawImage(QRectF(-offset, -offset, s, s), wavetableImage);
            } else if (params.shapeId == 0 && params.shapeEdgeFreq == 0) {
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
                    
                    // Apply Phase Warp (Distortion)
                    double t_warped = t;
                    if (params.shapeWarpAmp > 0 && params.shapeWarpFreq > 0) {
                        // sin(t * freq) creates a periodic shift in angle
                        // Amp controls how strong the shift is
                        double warp = std::sin(t * params.shapeWarpFreq);
                        double warpStrength = params.shapeWarpAmp / 50.0; // Scale to reasonable range (0.0 - 2.0 radians)
                        t_warped += warp * warpStrength;
                    }

                    // Base Shape Radius
                    double currentR = radius;
                    
                    if (n > 0) {
                         // Regular Polygon Polar Formula
                         // Use warped t for "Liquify" effect on the polygon itself
                         double an = 2 * M_PI / n;
                         double t_rot = t_warped + rotationOffset; 
                         
                         // We want fmod(t_rot, an) - an/2
                         double he = std::fmod(t_rot, an);
                         if (he < 0) he += an;
                         he -= an / 2.0;
                         
                         double scale = std::cos(M_PI / n) / std::cos(he);
                         currentR *= scale; 
                    }

                    // Edge Modulation (FM Synthesis equivalent)
                    if (params.shapeEdgeFreq > 0 && params.shapeEdgeAmp > 0) {
                        // Use warped t for the wave too, creating non-uniform spikes
                        double wave = std::sin(t_warped * params.shapeEdgeFreq);
                        double phase = i * 13.5; 
                        wave = std::sin(t_warped * params.shapeEdgeFreq + phase);
                        
                        double ampFactor = params.shapeEdgeAmp / 100.0;
                        currentR *= (1.0 + wave * ampFactor);
                    }

                    double px = currentR * std::cos(t); // Keep original t for position to maintain continuity? 
                    // No, if we want the shape to actually warp, we should probably use t for position 
                    // BUT polar coords: (r, theta). If we change r based on t_warped, but plot at t, we get radial distortion.
                    // If we plot at t_warped, we get angular bunching.
                    // Let's stick to plotting at 't' but calculating radius based on 't_warped'.
                    // This creates the "Twist" effect on the shape's features without breaking the circle loop.
                    
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
