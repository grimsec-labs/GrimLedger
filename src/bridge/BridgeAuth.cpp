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

QString endpointName() {
    const QByteArray seed = QByteArray("grimledger-bridge-v2");
    const QByteArray hash = QCryptographicHash::hash(seed, QCryptographicHash::Sha256);
    return QStringLiteral("grimledger-") + hash.toHex().left(16);
}

QByteArray generateToken() {
    QByteArray token(32, '\0');
    randombytes_buf(token.data(), static_cast<size_t>(token.size()));
    return token;
}

bool writeSessionToken(const QByteArray& token) {
    if (token.size() < 16) {
        return false;
    }
    QFile file(tokenFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(token) == token.size();
}

bool readSessionToken(QByteArray& tokenOut) {
    QFile file(tokenFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    tokenOut = file.readAll();
    return tokenOut.size() >= 16;
}

void clearSessionToken() {
    QFile::remove(tokenFilePath());
}

bool constantTimeEquals(const QByteArray& a, const QByteArray& b) {
    if (a.size() != b.size() || a.isEmpty()) {
        return false;
    }
    return sodium_memcmp(a.constData(), b.constData(), static_cast<size_t>(a.size())) == 0;
}

} // namespace BridgeAuth
