#include "share/GrimShare.h"
#include "security/CryptoManager.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace GrimShare {

namespace {

constexpr char kMagic[] = "GRIMSHR1";

QByteArray serializePayload(const QVector<GrimShareNotePayload>& notes) {
    QJsonArray arr;
    for (const GrimShareNotePayload& note : notes) {
        QJsonObject obj;
        obj.insert(QStringLiteral("title"), note.title);
        obj.insert(QStringLiteral("body"), note.body);
        QJsonArray tags;
        for (const QString& tag : note.tags) {
            tags.append(tag);
        }
        obj.insert(QStringLiteral("tags"), tags);
        arr.append(obj);
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("notes"), arr);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

} // namespace

GrimSharePackResult packNotes(
    const QString& outputPath,
    const QVector<GrimShareNotePayload>& notes,
    const QString& passphrase) {
    GrimSharePackResult result;
    if (notes.isEmpty() || passphrase.isEmpty()) {
        result.error = QStringLiteral("Notes and passphrase are required.");
        return result;
    }

    const QByteArray salt = CryptoManager::randomBytes(CryptoManager::kSaltSize);
    const auto params = CryptoManager::defaultKdfParams();
    auto key = CryptoManager::deriveKey(passphrase, salt, params);
    if (!key) {
        result.error = QStringLiteral("Could not derive share key.");
        return result;
    }

    const QByteArray header = salt + QByteArray(reinterpret_cast<const char*>(&params.opsLimit), 8)
        + QByteArray(reinterpret_cast<const char*>(&params.memLimit), 8);
    const QByteArray plaintext = serializePayload(notes);
    const auto ciphertext = CryptoManager::encryptAead(
        plaintext, *key, CryptoManager::grimShareAssociatedData(header));
    CryptoManager::secureZero(*key);
    if (!ciphertext) {
        result.error = QStringLiteral("Could not encrypt share package.");
        return result;
    }

    QFile out(outputPath);
    if (!out.open(QIODevice::WriteOnly)) {
        result.error = QStringLiteral("Could not write share file.");
        return result;
    }
    out.write(kMagic, 8);
    out.write(header);
    out.write(*ciphertext);
    out.close();
    result.ok = true;
    return result;
}

GrimShareUnpackResult unpackFile(const QString& inputPath, const QString& passphrase) {
    GrimShareUnpackResult result;
    QFile in(inputPath);
    if (!in.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("Could not open share file.");
        return result;
    }

    char magic[8] = {};
    if (in.read(magic, 8) != 8 || qstrncmp(magic, kMagic, 8) != 0) {
        result.error = QStringLiteral("Not a GrimShare package.");
        return result;
    }

    const QByteArray header = in.read(CryptoManager::kSaltSize + 16);
    const QByteArray ciphertext = in.readAll();
    if (header.size() < CryptoManager::kSaltSize || ciphertext.isEmpty()) {
        result.error = QStringLiteral("Corrupt GrimShare package.");
        return result;
    }

    const QByteArray salt = header.left(CryptoManager::kSaltSize);
    CryptoManager::KdfParams params = CryptoManager::defaultKdfParams();
    auto key = CryptoManager::deriveKey(passphrase, salt, params);
    if (!key) {
        result.error = QStringLiteral("Could not derive share key.");
        return result;
    }

    const auto plaintext = CryptoManager::decryptAead(
        ciphertext, *key, CryptoManager::grimShareAssociatedData(header));
    CryptoManager::secureZero(*key);
    if (!plaintext) {
        result.error = QStringLiteral("Wrong passphrase or corrupt package.");
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(*plaintext);
    if (!doc.isObject()) {
        result.error = QStringLiteral("Invalid share payload.");
        return result;
    }
    const QJsonArray notes = doc.object().value(QStringLiteral("notes")).toArray();
    for (const QJsonValue& value : notes) {
        const QJsonObject obj = value.toObject();
        GrimShareNotePayload note;
        note.title = obj.value(QStringLiteral("title")).toString();
        note.body = obj.value(QStringLiteral("body")).toString();
        for (const QJsonValue& tag : obj.value(QStringLiteral("tags")).toArray()) {
            note.tags.append(tag.toString());
        }
        result.notes.append(note);
    }
    result.ok = true;
    return result;
}

} // namespace GrimShare
