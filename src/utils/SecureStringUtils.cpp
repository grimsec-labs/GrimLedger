#include "utils/SecureStringUtils.h"
#include "security/CryptoManager.h"

void SecureStringUtils::clearQString(QString& str) {
    if (!str.isEmpty()) {
        str.fill(QChar(u'\0'));
    }
    str.clear();
}

QString SecureStringUtils::fromUtf8Secure(const QByteArray& data) {
    return QString::fromUtf8(data);
}

QByteArray SecureStringUtils::toUtf8Secure(const QString& str) {
    return str.toUtf8();
}
