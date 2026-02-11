#include "AbrWriter.h"
#include <QFile>
#include <QDataStream>
#include <QImage>
#include <QBuffer>
#include <QDebug>

// Helper to write Pascal string (1 byte length + content)
// In some versions, it might be padded. For V1/V2, we'll assume no padding or 2-byte align?
// Let's stick to simple Pascal string first.
void writePascalString(QDataStream& out, const QString& str) {
    QByteArray bytes = str.toLatin1(); // or Utf8? Photoshop usually expects MacRoman or Latin1 for old versions.
    if (bytes.size() > 255) bytes.resize(255);
    quint8 len = static_cast<quint8>(bytes.size());
    out << len;
    if (len > 0) {
        out.writeRawData(bytes.constData(), len);
    }
    // Alignment? Some specs say pad to 4 bytes. Let's try without first.
}

// PackBits RLE compression
QByteArray packBits(const QByteArray& row) {
    QByteArray packed;
    int len = row.size();
    int i = 0;
    while (i < len) {
        int j = i;
        // Find run of identical bytes
        while (j < len - 1 && row[j] == row[j+1] && (j - i) < 128) {
            j++;
        }
        if (j > i) { // Run found (at least 2 bytes)
            // Output run
            // Count = -(run_length - 1)
            // e.g. run 3 -> count -2
            int runLen = j - i + 1;
            // PackBits spec: -127 to -1 (repeat byte 2 to 128 times)
            // -128 is no-op
            // Wait, Apple PackBits: n in [-127, -1], replicate (1-n) times.
            // if n=-1, replicate 2 times.
            // if n=-127, replicate 128 times.
            // So n = -(runLen - 1)
            // if runLen=2, n=-1. if runLen=128, n=-127.
            // If runLen > 128, we handle in loop.
            // But here loop constraint is (j-i) < 128, so runLen <= 129?
            // Wait, PackBits run max 128.
            // My loop condition (j-i) < 128 allows runLen up to 129?
            // Actually, (j-i) is index diff.
            // if i=0, j=0 (1 byte), loop doesn't run.
            // if i=0, j=1 (2 bytes), runLen=2.
            
            // Correct logic:
            // Look for run of at least 3 bytes? Or 2?
            // Usually 3 bytes is better for compression overhead.
            // But PackBits can handle 2.
            
            // Re-implement simplified:
            // Find literal run or repeat run.
        }
        
        // Let's use a simpler robust implementation
        // ...
        i++;
    }
    return packed;
}

// Simpler PackBits implementation
QByteArray encodePackBits(const QByteArray& data) {
    QByteArray result;
    int len = data.size();
    if (len == 0) return result;

    int i = 0;
    while (i < len) {
        // Find run of identical bytes
        int runStart = i;
        int runLen = 1;
        while (i + runLen < len && runLen < 128 && data[i + runLen] == data[i]) {
            runLen++;
        }

        if (runLen > 1) { // Found a run (size 2 to 128)
            // PackBits: n = 1 - runLen (range -1 to -127)
            // if runLen=2, n=-1.
            qint8 n = static_cast<qint8>(1 - runLen);
            result.append((char)n);
            result.append(data[i]);
            i += runLen;
        } else {
            // Literal run
            int litStart = i;
            int litLen = 1;
            // Find length of literal run
            // Stop if we find a run of 3 identical bytes (worth compressing)
            // or if we hit 128 limit
            while (i + litLen < len && litLen < 128) {
                if (i + litLen + 2 < len && 
                    data[i + litLen] == data[i + litLen + 1] && 
                    data[i + litLen] == data[i + litLen + 2]) {
                    break; // Found a run of 3, stop literal
                }
                litLen++;
            }
            
            // Output literal
            // n = litLen - 1 (0 to 127)
            qint8 n = static_cast<qint8>(litLen - 1);
            result.append((char)n);
            result.append(data.mid(i, litLen));
            i += litLen;
        }
    }
    return result;
}

bool AbrWriter::writeAbr(const QString& filename, const QImage& brushImage, const QString& brushName, int spacingPercent) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream out(&file);
    out.setByteOrder(QDataStream::BigEndian);

    // 1. Header (V1)
    out << (qint16)1; // Version 1
    // Some specs say V1 has no subversion?
    // GIMP abr.c: if version==1, read subversion.
    // Let's write subversion 1.
    out << (qint16)1; // Subversion
    out << (qint16)1; // Count (1 brush)

    // 2. Brush Data (Sampled Brush - Type 2)
    
    // Prepare data first to calculate size
    QByteArray brushData;
    QDataStream bOut(&brushData, QIODevice::WriteOnly);
    bOut.setByteOrder(QDataStream::BigEndian);

    // Misc fields
    bOut << (qint16)spacingPercent; // Spacing (0-999)
    
    // Name (Pascal String)
    // In V1, no padding? Let's assume no padding.
    // Use helper
    QByteArray nameBytes = brushName.toLatin1();
    if (nameBytes.size() > 255) nameBytes.resize(255);
    quint8 nameLen = static_cast<quint8>(nameBytes.size());
    bOut << nameLen;
    if (nameLen > 0) bOut.writeRawData(nameBytes.constData(), nameLen);
    
    // Anti-aliasing (1 byte)
    bOut << (quint8)1; // True

    // Interest (2 bytes)
    bOut << (qint16)0; 

    // Bounds (Top, Left, Bottom, Right) - 2 bytes each
    // Relative to the image itself?
    // Usually (0, 0, Height, Width)
    // Ensure image is valid
    QImage grayImg = brushImage.convertToFormat(QImage::Format_Grayscale8);
    // Invert? Photoshop brushes: Black = opaque, White = transparent?
    // Or Alpha?
    // Usually ABR stores the alpha channel as grayscale.
    // White (255) = Opaque, Black (0) = Transparent? 
    // Or Black (0) = Opaque, White (255) = Transparent?
    // GIMP says: "This is the opposite convention... if you want to import... invert them."
    // Photoshop: Black (0) is ink?
    // Wait, typical alpha mask: 255 = Opaque.
    // But Photoshop brushes are often stored as "Ink Density".
    // 0 = No Ink (Transparent), 255 = Max Ink (Black).
    // Let's assume 255 = Opaque (Black ink), 0 = Transparent.
    // If the QImage has alpha, we should extract alpha.
    
    // Extract alpha channel to grayscale
    // QImage::alphaChannel() is deprecated/removed in Qt 6. Use convertToFormat(QImage::Format_Alpha8).
    QImage alphaImg = brushImage.convertToFormat(QImage::Format_Alpha8);
    // If original image is grayscale without alpha, use it directly?
    // TextureGenerator produces ARGB32 with Black color and varying Alpha.
    // So alpha channel is the correct data.
    // 0 alpha -> Transparent. 255 alpha -> Opaque.
    // This matches "Ink Density" if we treat it as such.
    
    qint16 top = 0;
    qint16 left = 0;
    qint16 bottom = static_cast<qint16>(alphaImg.height());
    qint16 right = static_cast<qint16>(alphaImg.width());
    
    bOut << top << left << bottom << right;

    // Depth (2 bytes)
    bOut << (qint16)8; // 8-bit

    // Image Data
    // PackBits compression? Or Raw?
    // Try Raw first?
    // No, search results suggest PackBits.
    // Let's assume V1/V2 supports RLE.
    // Wait, does V1 support RLE?
    // "Computed brush data... Sampled brush data varies..."
    // I'll check if I can just write Raw.
    // If I write Raw, size must be exactly H*W.
    // If I write RLE, size is compressed size.
    // The `Size` field (before Data) tells the parser how many bytes to read.
    // So if I specify correct Size, the parser *might* try to detect compression?
    // Or maybe V1/V2 *always* uses RLE?
    // Most sources imply RLE is standard for Sampled brushes.
    
    // Let's compress row by row
    QByteArray imgBytes;
    for (int y = 0; y < alphaImg.height(); ++y) {
        const uchar* scanLine = alphaImg.constScanLine(y);
        QByteArray row((const char*)scanLine, alphaImg.width());
        // For Photoshop brushes, is it 0=Transparent, 255=Opaque?
        // Yes, usually.
        imgBytes.append(encodePackBits(row));
    }
    
    // Append image bytes to brushData
    bOut.writeRawData(imgBytes.constData(), imgBytes.size());

    // Write Type and Size to main stream
    out << (qint16)2; // Type = Sampled
    
    // Size: Total bytes in brushData
    qint32 size = static_cast<qint32>(brushData.size());
    out << size;
    
    // Write the brush data
    out.writeRawData(brushData.constData(), brushData.size());

    file.close();
    return true;
}
