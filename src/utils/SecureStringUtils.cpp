#include "utils/SecureStringUtils.h"
#include "security/CryptoManager.h"

void SecureStringUtils::clearQString(QString& str) {
    // GL-SEC-006: best-effort wipe — overwrite this instance's character buffer in
    // place before releasing it, instead of the previous no-op (`str = QString()`).
    // LIMITATIONS (documented, not solved here): QString is implicitly shared and
    // immutable-by-design, so prior *copies* are not reached and the freed heap page
    // may still be recoverable from swap/dumps. Truly sensitive material (the derived
    // key, decrypted passwords) should be held in QByteArray and zeroed via
    // CryptoManager::secureZero, or in sodium_malloc/sodium_mlock buffers.
    if (!str.isEmpty()) {
        str.fill(QChar(u'\0'));  // detaches if shared, then overwrites this buffer
    }
    str.clear();
}

QString SecureStringUtils::fromUtf8Secure(const QByteArray& data) {
    return QString::fromUtf8(data);
}

QByteArray SecureStringUtils::toUtf8Secure(const QString& str) {
    return str.toUtf8();
}
