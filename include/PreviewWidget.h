#pragma once

#include <QWidget>
#include <QImage>
#include <QPainter>

class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget* parent = nullptr) : QWidget(parent) {
        // Set a minimum size to ensure it's visible
        setMinimumSize(200, 200);
        // Set background color to grey for better visibility of transparent brushes
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("background-color: #ccc; border: 1px solid #999;");
    }

    void setImage(const QImage& image) {
        m_image = image;
        update(); // Trigger repaint
    }

    const QImage& image() const {
        return m_image;
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        
        // Draw background (handled by stylesheet, but we can draw a grid if we want later)
        
        if (m_image.isNull()) {
            painter.drawText(rect(), Qt::AlignCenter, "No Preview");
            return;
        }

        // Calculate scaled rect to keep aspect ratio
        // Add some padding (10px) so the image doesn't touch the widget border
        int padding = 10;
        QSize widgetSize = size() - QSize(padding * 2, padding * 2);
        if (widgetSize.width() <= 0 || widgetSize.height() <= 0) return;

        QSize imageSize = m_image.size();
        
        if (imageSize.isEmpty()) return;

        QSize scaledSize = imageSize.scaled(widgetSize, Qt::KeepAspectRatio);
        
        // Center the image (account for padding)
        int x = (width() - scaledSize.width()) / 2;
        int y = (height() - scaledSize.height()) / 2;
        
        QRect targetRect(x, y, scaledSize.width(), scaledSize.height());
        
        // Use smooth transformation for better quality
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        
        painter.drawImage(targetRect, m_image);
    }

private:
    QImage m_image;
};
