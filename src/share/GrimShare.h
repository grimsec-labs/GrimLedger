#pragma once

#include <QString>
#include <QVector>

struct GrimShareNotePayload {
    QString title;
    QString body;
    QVector<QString> tags;
};

struct GrimSharePackResult {
    bool ok = false;
    QString error;
};

struct GrimShareUnpackResult {
    bool ok = false;
    QString error;
    QVector<GrimShareNotePayload> notes;
};

namespace GrimShare {

GrimSharePackResult packNotes(
    const QString& outputPath,
    const QVector<GrimShareNotePayload>& notes,
    const QString& passphrase);
GrimShareUnpackResult unpackFile(const QString& inputPath, const QString& passphrase);

} // namespace GrimShare
