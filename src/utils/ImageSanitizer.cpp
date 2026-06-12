#include "utils/ImageSanitizer.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QSize>

namespace {

constexpr int kJpegQualityHigh = 90;
constexpr int kJpegQualityFloor = 82;
constexpr int kJpegQualityStep = 2;
constexpr int kDownscaleMaxDim = 4096;

bool encodeImage(const QImage& image, const char* format, int quality,
                 QByteArray& out, QString* error) {
    QBuffer buffer(&out);
    if (!buffer.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("Could not allocate image buffer.");
        return false;
    }

    QImageWriter writer(&buffer, format);
    if (quality > 0)
        writer.setQuality(quality);
    if (QByteArray(format) == "PNG")
        writer.setCompression(6);

    if (!writer.write(image)) {
        if (error)
            *error = QString("Could not encode image as %1.").arg(format);
        return false;
    }
    return true;
}

QImage buildCleanImage(const QImage& source) {
    QImage raster = source;
    if (!raster.hasAlphaChannel()) {
        raster = raster.convertToFormat(QImage::Format_RGB32);
    } else {
        raster = raster.convertToFormat(QImage::Format_RGBA8888);
    }

    QImage clean(raster.constBits(), raster.width(), raster.height(),
                 raster.bytesPerLine(), raster.format());
    return clean.copy();
}

bool tryEncode(const QImage& clean, bool hasAlpha, QByteArray& encoded,
               QString& mimeType, QString& ext, QString* error) {
    if (hasAlpha) {
        if (!encodeImage(clean, "PNG", -1, encoded, error))
            return false;
        mimeType = QStringLiteral("image/png");
        ext = QStringLiteral(".png");
        return true;
    }

    for (int q = kJpegQualityHigh; q >= kJpegQualityFloor; q -= kJpegQualityStep) {
        encoded.clear();
        if (!encodeImage(clean, "JPEG", q, encoded, error))
            return false;
        if (encoded.size() <= ImageSanitizer::kMaxOutputBytes) {
            mimeType = QStringLiteral("image/jpeg");
            ext = QStringLiteral(".jpg");
            return true;
        }
    }

    encoded.clear();
    if (!encodeImage(clean, "JPEG", kJpegQualityFloor, encoded, error))
        return false;
    mimeType = QStringLiteral("image/jpeg");
    ext = QStringLiteral(".jpg");
    return true;
}

bool sanitizeImage(const QImage& source, qint64 originalBytes,
                   SanitizedImage& out, QString* error) {
    if (source.isNull()) {
        if (error)
            *error = QStringLiteral("Unsupported or corrupt image file.");
        return false;
    }

    const qint64 pixels = static_cast<qint64>(source.width()) * source.height();
    if (pixels > ImageSanitizer::kMaxPixels) {
        if (error) {
            *error = QStringLiteral("Image dimensions are too large to decode safely. "
                                    "Maximum is %1 megapixels.")
                         .arg(ImageSanitizer::kMaxPixels / 1000000);
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

    const bool hasAlpha = raster.hasAlphaChannel();
    QImage clean = buildCleanImage(raster);

    QByteArray encoded;
    QString mimeType, ext;
    if (!tryEncode(clean, hasAlpha, encoded, mimeType, ext, error))
        return false;

    if (encoded.size() > ImageSanitizer::kMaxOutputBytes && !hasAlpha) {
        QImage scaled = clean.scaled(kDownscaleMaxDim, kDownscaleMaxDim,
                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);
        scaled = buildCleanImage(scaled);
        encoded.clear();
        if (!tryEncode(scaled, false, encoded, mimeType, ext, error))
            return false;
        clean = scaled;
    }

    if (encoded.size() > ImageSanitizer::kMaxOutputBytes) {
        if (error)
            *error = QStringLiteral("Image is too large after sanitization. Try a smaller image.");
        return false;
    }

    out.data = std::move(encoded);
    out.mimeType = mimeType;
    out.suggestedExtension = ext;
    out.width = clean.width();
    out.height = clean.height();
    out.originalBytes = originalBytes;
    out.sanitizedBytes = out.data.size();
    return true;
}

bool sanitizeFromReader(QImageReader& reader, qint64 originalBytes,
                        SanitizedImage& out, QString* error) {
    reader.setAutoTransform(true);

    const QSize size = reader.size();
    if (size.isValid()) {
        const qint64 pixels = static_cast<qint64>(size.width()) * size.height();
        if (pixels > ImageSanitizer::kMaxPixels) {
            if (error) {
                *error = QStringLiteral("Image dimensions are too large to decode safely. "
                                        "Maximum is %1 megapixels.")
                             .arg(ImageSanitizer::kMaxPixels / 1000000);
            }
            return false;
        }

        if (size.width() > ImageSanitizer::kMaxDimension
            || size.height() > ImageSanitizer::kMaxDimension) {
            reader.setScaledSize(size.scaled(
                ImageSanitizer::kMaxDimension, ImageSanitizer::kMaxDimension,
                Qt::KeepAspectRatio));
        }
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

    return sanitizeImage(image, originalBytes, out, error);
}

} // namespace

bool ImageSanitizer::sanitizeFromFile(const QString& path, SanitizedImage& out, QString* error) {
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        if (error)
            *error = QStringLiteral("Image file not found.");
        return false;
    }

    const qint64 fileSize = info.size();
    if (fileSize > kMaxInputBytes) {
        if (error) {
            *error = QStringLiteral("Image file is too large to import safely. "
                                    "Maximum input size is %1 MB.")
                         .arg(kMaxInputBytes / (1024 * 1024));
        }
        return false;
    }

    QImageReader reader(path);
    return sanitizeFromReader(reader, fileSize, out, error);
}

bool ImageSanitizer::sanitizeFromData(const QByteArray& data, SanitizedImage& out, QString* error) {
    if (data.isEmpty()) {
        if (error)
            *error = QStringLiteral("The selected image is empty.");
        return false;
    }

    if (data.size() > kMaxInputBytes) {
        if (error) {
            *error = QStringLiteral("Image file is too large to import safely. "
                                    "Maximum input size is %1 MB.")
                         .arg(kMaxInputBytes / (1024 * 1024));
        }
        return false;
    }

    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Could not read the selected image.");
        return false;
    }

    QImageReader reader(&buffer);
    return sanitizeFromReader(reader, data.size(), out, error);
}

QString ImageSanitizer::extensionForMime(const QString& mimeType) {
    if (mimeType == QStringLiteral("image/png"))
        return QStringLiteral(".png");
    if (mimeType == QStringLiteral("image/jpeg"))
        return QStringLiteral(".jpg");
    if (mimeType == QStringLiteral("image/webp"))
        return QStringLiteral(".webp");
    return QStringLiteral(".bin");
}
