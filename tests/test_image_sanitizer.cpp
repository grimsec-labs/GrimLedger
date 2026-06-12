// Security regression test for ImageSanitizer.
//
// The sanitizer strips image metadata (EXIF/GPS/comments) before encrypting into
// the vault. This test proves metadata doesn't survive, that normal images of
// various sizes import successfully, and that unsafe input is rejected.

#include "utils/ImageSanitizer.h"

#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QImage>
#include <QImageWriter>
#include <QString>

namespace {

QByteArray makePngWithMetadata(const QString& sentinel, int w = 8, int h = 8) {
    QImage img(w, h, QImage::Format_RGB32);
    img.fill(Qt::red);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly))
        return {};
    QImageWriter writer(&buffer, "PNG");
    writer.setText(QStringLiteral("Author"), sentinel);
    writer.setText(QStringLiteral("Comment"), QStringLiteral("GPS ") + sentinel);
    if (!writer.write(img))
        return {};
    buffer.close();
    return bytes;
}

QByteArray makeJpegImage(int w, int h) {
    QImage img(w, h, QImage::Format_RGB32);
    img.fill(Qt::blue);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly))
        return {};
    QImageWriter writer(&buffer, "JPEG");
    writer.setQuality(85);
    if (!writer.write(img))
        return {};
    buffer.close();
    return bytes;
}

QByteArray makePngWithAlpha(int w = 64, int h = 64) {
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(QColor(255, 0, 0, 128));

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly))
        return {};
    QImageWriter writer(&buffer, "PNG");
    if (!writer.write(img))
        return {};
    buffer.close();
    return bytes;
}

int fail(const char* what) {
    qWarning("test_image_sanitizer FAILED: %s", what);
    return 1;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    const QString sentinel = QStringLiteral("SECRET_EXIF_GPS_MARKER_42");

    // --- 1. Metadata purge: sanitized output carries no metadata. ---
    {
        const QByteArray source = makePngWithMetadata(sentinel);
        if (source.isEmpty())
            return fail("could not build source PNG with metadata");

        const QImage sourceDecoded = QImage::fromData(source, "PNG");
        if (sourceDecoded.text(QStringLiteral("Author")) != sentinel)
            return fail("setup: source PNG did not retain injected metadata");

        SanitizedImage out;
        QString error;
        if (!ImageSanitizer::sanitizeFromData(source, out, &error))
            return fail("sanitizeFromData rejected a valid image");
        if (out.data.isEmpty())
            return fail("sanitized output is empty");
        if (out.data.contains(sentinel.toUtf8()))
            return fail("metadata sentinel survived in sanitized bytes");

        const QImage outDecoded = QImage::fromData(out.data);
        if (outDecoded.isNull())
            return fail("sanitized output is not a valid image");
        if (!outDecoded.text(QStringLiteral("Author")).isEmpty()
            || !outDecoded.text(QStringLiteral("Comment")).isEmpty())
            return fail("text metadata survived sanitization");
        if (outDecoded.width() != 8 || outDecoded.height() != 8)
            return fail("sanitized image dimensions changed unexpectedly");
    }

    // --- 2. Small image import succeeds ---
    {
        SanitizedImage out;
        const QByteArray small = makeJpegImage(100, 100);
        if (small.isEmpty())
            return fail("could not create small JPEG");
        if (!ImageSanitizer::sanitizeFromData(small, out, nullptr))
            return fail("small JPEG should sanitize successfully");
        if (out.data.isEmpty())
            return fail("small JPEG produced empty output");
    }

    // --- 3. 500 KB-class image regression (previously failed as PNG) ---
    {
        SanitizedImage out;
        const QByteArray medium = makeJpegImage(1920, 1080);
        if (medium.isEmpty())
            return fail("could not create 1920x1080 JPEG");
        if (!ImageSanitizer::sanitizeFromData(medium, out, nullptr))
            return fail("1920x1080 image should sanitize successfully (was the 500KB regression)");
        if (out.sanitizedBytes > ImageSanitizer::kMaxOutputBytes)
            return fail("1920x1080 sanitized output exceeds max output limit");
    }

    // --- 4. Large smartphone image regression (4000x3000) ---
    {
        SanitizedImage out;
        const QByteArray big = makeJpegImage(4000, 3000);
        if (big.isEmpty())
            return fail("could not create 4000x3000 JPEG");
        if (!ImageSanitizer::sanitizeFromData(big, out, nullptr))
            return fail("4000x3000 smartphone image should sanitize successfully");
        if (out.sanitizedBytes > ImageSanitizer::kMaxOutputBytes)
            return fail("4000x3000 sanitized output exceeds max output limit");
    }

    // --- 5. Alpha image stays PNG ---
    {
        SanitizedImage out;
        const QByteArray alpha = makePngWithAlpha();
        if (alpha.isEmpty())
            return fail("could not create alpha PNG");
        if (!ImageSanitizer::sanitizeFromData(alpha, out, nullptr))
            return fail("alpha PNG should sanitize successfully");
        if (out.mimeType != QStringLiteral("image/png"))
            return fail("alpha image should remain PNG");
    }

    // --- 6. Non-alpha photo becomes JPEG ---
    {
        SanitizedImage out;
        const QByteArray photo = makeJpegImage(640, 480);
        if (photo.isEmpty())
            return fail("could not create photo JPEG");
        if (!ImageSanitizer::sanitizeFromData(photo, out, nullptr))
            return fail("photo JPEG should sanitize successfully");
        if (out.mimeType != QStringLiteral("image/jpeg"))
            return fail("non-alpha photo should become JPEG, got: " + out.mimeType.toUtf8());
    }

    // --- 7. Oversize input rejected before decode ---
    {
        SanitizedImage out;
        QString error;
        QByteArray huge;
        huge.fill('\x89', ImageSanitizer::kMaxInputBytes + 1);
        if (ImageSanitizer::sanitizeFromData(huge, out, &error))
            return fail("sanitizeFromData accepted oversize input");
        if (!error.contains(QStringLiteral("too large")))
            return fail("oversize error should mention 'too large'");
    }

    // --- 8. Malformed input rejected (no crash) ---
    {
        SanitizedImage out;
        const QByteArray garbage = QByteArrayLiteral("this is definitely not an image file");
        if (ImageSanitizer::sanitizeFromData(garbage, out, nullptr))
            return fail("sanitizeFromData accepted non-image garbage");
    }

    // --- 9. Empty input rejected ---
    {
        SanitizedImage out;
        if (ImageSanitizer::sanitizeFromData(QByteArray(), out, nullptr))
            return fail("sanitizeFromData accepted empty input");
    }

    // --- 10. SanitizedImage metadata fields populated ---
    {
        SanitizedImage out;
        const QByteArray src = makeJpegImage(320, 240);
        if (!ImageSanitizer::sanitizeFromData(src, out, nullptr))
            return fail("sanitizeFromData failed on 320x240");
        if (out.width != 320 || out.height != 240)
            return fail("SanitizedImage width/height not set correctly");
        if (out.originalBytes != src.size())
            return fail("SanitizedImage originalBytes not set");
        if (out.sanitizedBytes != out.data.size())
            return fail("SanitizedImage sanitizedBytes mismatch");
        if (out.suggestedExtension.isEmpty())
            return fail("SanitizedImage suggestedExtension empty");
    }

    // --- 11. extensionForMime helper ---
    {
        if (ImageSanitizer::extensionForMime("image/png") != ".png")
            return fail("extensionForMime wrong for PNG");
        if (ImageSanitizer::extensionForMime("image/jpeg") != ".jpg")
            return fail("extensionForMime wrong for JPEG");
        if (ImageSanitizer::extensionForMime("image/webp") != ".webp")
            return fail("extensionForMime wrong for WebP");
        if (ImageSanitizer::extensionForMime("application/octet-stream") != ".bin")
            return fail("extensionForMime wrong for unknown");
    }

    return 0;
}
