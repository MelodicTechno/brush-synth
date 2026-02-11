#pragma once

#include <QImage>
#include <QPainter>
#include <QRandomGenerator>

class TextureGenerator {
public:
    struct Parameters {
        int canvasSize;
        int count;
        int minSize;
        int maxSize;
        int minOpacity;
        int maxOpacity;
    };

    static QImage generate(const Parameters& params) {
        QImage image(params.canvasSize, params.canvasSize, QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);

        auto* rng = QRandomGenerator::global();

        for (int i = 0; i < params.count; ++i) {
            int s = rng->bounded(params.minSize, params.maxSize + 1);
            int alpha = rng->bounded(params.minOpacity, params.maxOpacity + 1);
            int x = rng->bounded(0, params.canvasSize);
            int y = rng->bounded(0, params.canvasSize);

            QColor color(0, 0, 0, alpha);
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPoint(x, y), s / 2, s / 2);
        }

        return image;
    }
};
