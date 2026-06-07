#include "utils/SecureStringUtils.h"
#include "security/CryptoManager.h"

void SecureStringUtils::clearQString(QString& str) {
    // QString is implicitly shared; replace with empty to release reference.
    str = QString();
}

QString SecureStringUtils::fromUtf8Secure(const QByteArray& data) {
    return QString::fromUtf8(data);
}

QByteArray SecureStringUtils::toUtf8Secure(const QString& str) {
    return str.toUtf8();
}
