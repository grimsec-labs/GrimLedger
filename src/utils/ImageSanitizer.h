#pragma once

#include <QByteArray>
#include <QString>

struct SanitizedImage {
    QByteArray data;
    QString mimeType;
    QString suggestedExtension;
    int width = 0;
    int height = 0;
    qint64 originalBytes = 0;
    qint64 sanitizedBytes = 0;
};

class ImageSanitizer {
public:
    static constexpr qint64 kMaxInputBytes = 50LL * 1024 * 1024;
    static constexpr qint64 kMaxOutputBytes = 25LL * 1024 * 1024;
    static constexpr int kMaxDimension = 8192;
    static constexpr qint64 kMaxPixels = 50LL * 1000 * 1000;

    static bool sanitizeFromFile(const QString& path, SanitizedImage& out, QString* error = nullptr);
    static bool sanitizeFromData(const QByteArray& data, SanitizedImage& out, QString* error = nullptr);

    static QString extensionForMime(const QString& mimeType);
};
