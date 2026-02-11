#ifndef ABRWRITER_H
#define ABRWRITER_H

#include <QString>
#include <QImage>

class AbrWriter {
public:
    static bool writeAbr(const QString& filename, const QImage& brushImage, const QString& brushName, int spacingPercent = 25);
};

#endif // ABRWRITER_H
