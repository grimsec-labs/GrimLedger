#include "utils/ImageSanitizer.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QSize>

namespace {

bool encodePng(const QImage& image, QByteArray& pngOut, QString* error) {
    QBuffer buffer(&pngOut);
    if (!buffer.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = QStringLiteral("Could not allocate image buffer.");
        }
        return false;
    }

    QImageWriter writer(&buffer, "PNG");
    writer.setCompression(6);
    if (!writer.write(image)) {
        if (error) {
            *error = QStringLiteral("Could not encode image as PNG.");
        }
        return false;
    }
    return true;
}

bool sanitizeImage(const QImage& source, SanitizedImage& out, QString* error) {
    if (source.isNull()) {
        if (error) {
            *error = QStringLiteral("Invalid or unsupported image.");
        }
        return false;
    }

    QImage raster = source;
    if (raster.width() > ImageSanitizer::kMaxDimension
        || raster.height() > ImageSanitizer::kMaxDimension) {
        raster = raster.scaled(
            ImageSanitizer::kMaxDimension,
            ImageSanitizer::kMaxDimension,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
    }

    if (!raster.hasAlphaChannel()) {
        raster = raster.convertToFormat(QImage::Format_RGB32);
    } else {
        raster = raster.convertToFormat(QImage::Format_RGBA8888);
    }

    QByteArray png;
    if (!encodePng(raster, png, error)) {
        return false;
    }

    if (png.size() > ImageSanitizer::kMaxOutputBytes) {
        if (error) {
            *error = QStringLiteral("Image is too large after sanitization.");
        }
        return false;
    }

    out.pngData = std::move(png);
    out.width = raster.width();
    out.height = raster.height();
    return true;
}

} // namespace

bool ImageSanitizer::sanitizeFromFile(const QString& path, SanitizedImage& out, QString* error) {
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        if (error) {
            *error = QStringLiteral("Image file not found.");
        }
        return false;
    }

    if (info.size() > kMaxInputBytes) {
        if (error) {
            *error = QStringLiteral("Image file is too large.");
        }
        return false;
    }

    QImageReader reader(path);
    reader.setAutoTransform(true);

    const QSize size = reader.size();
    if (!size.isValid()) {
        if (error) {
            *error = QStringLiteral("Could not read image file.");
        }
        return false;
    }

    if (size.width() > kMaxDimension || size.height() > kMaxDimension) {
        reader.setScaledSize(size.scaled(kMaxDimension, kMaxDimension, Qt::KeepAspectRatio));
    }

    const QImage image = reader.read();
    if (image.isNull()) {
        if (error) {
            *error = reader.errorString().isEmpty()
                ? QStringLiteral("Unsupported or corrupt image file.")
                : reader.errorString();
        }
        return false;
    }

    return sanitizeImage(image, out, error);
}
