#pragma once

#include <QByteArray>
#include <QString>

struct SanitizedImage {
    QByteArray pngData;
    int width = 0;
    int height = 0;
};

class ImageSanitizer {
public:
    static constexpr int kMaxInputBytes = 15 * 1024 * 1024;
    static constexpr int kMaxOutputBytes = 10 * 1024 * 1024;
    static constexpr int kMaxDimension = 8192;

    // Decode pixels and re-encode as PNG to strip EXIF/metadata.
    static bool sanitizeFromFile(const QString& path, SanitizedImage& out, QString* error = nullptr);

    // Same pipeline as sanitizeFromFile, but operates on in-memory bytes. Used on
    // Android where the file picker hands back content bytes (no filesystem path).
    static bool sanitizeFromData(const QByteArray& data, SanitizedImage& out, QString* error = nullptr);
};
