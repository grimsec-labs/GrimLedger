// Security regression test for ImageSanitizer.
//
// The sanitizer is the single shared-core path that both desktop and Android use
// to strip image metadata (EXIF/GPS/comments) before an image is encrypted into
// the vault. This test proves that embedded metadata does NOT survive
// sanitization, and that malformed input is rejected without crashing.

#include "utils/ImageSanitizer.h"

#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QImage>
#include <QImageWriter>
#include <QString>

namespace {

// Build a PNG that carries text metadata (tEXt chunks) — a stand-in for the
// EXIF/GPS/comment blocks a real camera image would contain.
QByteArray makePngWithMetadata(const QString& sentinel) {
    QImage img(8, 8, QImage::Format_RGB32);
    img.fill(Qt::red);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return {};
    }
    QImageWriter writer(&buffer, "PNG");
    writer.setText(QStringLiteral("Author"), sentinel);
    writer.setText(QStringLiteral("Comment"), QStringLiteral("GPS ") + sentinel);
    if (!writer.write(img)) {
        return {};
    }
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

    // --- Setup sanity: the source image really does carry metadata. ---
    const QByteArray source = makePngWithMetadata(sentinel);
    if (source.isEmpty()) {
        return fail("could not build source PNG with metadata");
    }
    const QImage sourceDecoded = QImage::fromData(source, "PNG");
    if (sourceDecoded.text(QStringLiteral("Author")) != sentinel) {
        return fail("setup: source PNG did not retain injected metadata");
    }

    // --- 1. Metadata purge: sanitized output carries no metadata. ---
    SanitizedImage out;
    QString error;
    if (!ImageSanitizer::sanitizeFromData(source, out, &error)) {
        return fail("sanitizeFromData rejected a valid image");
    }
    if (out.pngData.isEmpty()) {
        return fail("sanitized output is empty");
    }
    // Byte-level: the sentinel must not appear anywhere in the output bytes.
    if (out.pngData.contains(sentinel.toUtf8())) {
        return fail("metadata sentinel survived in sanitized bytes");
    }
    // Image-level: re-decoded output must have no text metadata fields.
    const QImage outDecoded = QImage::fromData(out.pngData, "PNG");
    if (outDecoded.isNull()) {
        return fail("sanitized output is not a valid PNG");
    }
    if (!outDecoded.text(QStringLiteral("Author")).isEmpty()
        || !outDecoded.text(QStringLiteral("Comment")).isEmpty()) {
        return fail("text metadata survived sanitization");
    }
    if (outDecoded.width() != 8 || outDecoded.height() != 8) {
        return fail("sanitized image dimensions changed unexpectedly");
    }

    // --- 2. Malformed input is rejected cleanly (no crash, returns false). ---
    SanitizedImage badOut;
    const QByteArray garbage = QByteArrayLiteral("this is definitely not an image file");
    if (ImageSanitizer::sanitizeFromData(garbage, badOut, &error)) {
        return fail("sanitizeFromData accepted non-image garbage");
    }

    // --- 3. Empty input is rejected. ---
    SanitizedImage emptyOut;
    if (ImageSanitizer::sanitizeFromData(QByteArray(), emptyOut, &error)) {
        return fail("sanitizeFromData accepted empty input");
    }

    // --- 4. Oversize input is rejected without decoding. ---
    SanitizedImage hugeOut;
    QByteArray huge;
    huge.fill('\x89', ImageSanitizer::kMaxInputBytes + 1);
    if (ImageSanitizer::sanitizeFromData(huge, hugeOut, &error)) {
        return fail("sanitizeFromData accepted oversize input");
    }

    return 0;
}
