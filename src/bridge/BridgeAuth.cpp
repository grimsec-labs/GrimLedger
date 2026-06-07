#include "bridge/BridgeAuth.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <sodium.h>

namespace BridgeAuth {

QString sessionDirectory() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/bridge");
    QDir().mkpath(dir);
    return dir;
}

QString tokenFilePath() {
    return sessionDirectory() + QStringLiteral("/session.token");
}

QString endpointFilePath() {
    return sessionDirectory() + QStringLiteral("/session.endpoint");
}

QString endpointName() {
    const QByteArray seed = QByteArray("grimledger-bridge-v2");
    const QByteArray hash = QCryptographicHash::hash(seed, QCryptographicHash::Sha256);
    return QStringLiteral("grimledger-") + hash.toHex().left(16);
}

QString generateSessionEndpoint() {
    const QByteArray suffix = generateToken().left(8);
    return endpointName() + QStringLiteral("-") + QString::fromLatin1(suffix.toHex());
}

bool writeSessionEndpoint(const QString& endpoint) {
    if (endpoint.isEmpty()) {
        return false;
    }
    QFile file(endpointFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QByteArray bytes = endpoint.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        return false;
    }
#if defined(Q_OS_WIN)
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
#endif
    return true;
}

bool readSessionEndpoint(QString& endpointOut) {
    QFile file(endpointFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    endpointOut = QString::fromUtf8(file.readAll().trimmed());
    return !endpointOut.isEmpty();
}

void clearSessionEndpoint() {
    QFile::remove(endpointFilePath());
}

QByteArray generateToken() {
    QByteArray token(32, '\0');
    randombytes_buf(token.data(), static_cast<size_t>(token.size()));
    return token;
}

QString tokenToTransportString(const QByteArray& token) {
    return QString::fromLatin1(token.toHex());
}

std::optional<QByteArray> tokenFromTransportString(const QString& encoded) {
    const QByteArray trimmed = encoded.trimmed().toLatin1();
    if (trimmed.size() != 64) {
        return std::nullopt;
    }
    const QByteArray decoded = QByteArray::fromHex(trimmed);
    if (decoded.size() != 32) {
        return std::nullopt;
    }
    return decoded;
}

bool writeSessionToken(const QByteArray& token) {
    if (token.size() != 32) {
        return false;
    }
    QFile file(tokenFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QByteArray hex = token.toHex();
    if (file.write(hex) != hex.size()) {
        return false;
    }
#if defined(Q_OS_WIN)
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
#endif
    return true;
}

bool readSessionToken(QByteArray& tokenOut) {
    QFile file(tokenFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray hex = file.readAll().trimmed();
    const QByteArray decoded = QByteArray::fromHex(hex);
    if (decoded.size() != 32) {
        return false;
    }
    tokenOut = decoded;
    return true;
}

void clearSessionToken() {
    QFile::remove(tokenFilePath());
    clearSessionEndpoint();
}

bool constantTimeEquals(const QByteArray& a, const QByteArray& b) {
    if (a.size() != b.size() || a.isEmpty()) {
        return false;
    }
    return sodium_memcmp(a.constData(), b.constData(), static_cast<size_t>(a.size())) == 0;
}

} // namespace BridgeAuth
