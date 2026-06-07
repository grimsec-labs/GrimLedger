#include "utils/TotpGenerator.h"

#include <sodium.h>

#include <QDateTime>
#include <QRegularExpression>

namespace TotpGenerator {

namespace {

QByteArray decodeBase32(const QString& input) {
    static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    QString cleaned = input.trimmed().toUpper();
    cleaned.remove(QRegularExpression(QStringLiteral("[^A-Z2-7]")));

    QByteArray output;
    int buffer = 0;
    int bitsLeft = 0;
    for (const QChar ch : cleaned) {
        const int val = static_cast<int>(strchr(alphabet, ch.toLatin1()) - alphabet);
        if (val < 0) {
            return QByteArray();
        }
        buffer = (buffer << 5) | val;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            bitsLeft -= 8;
            output.append(static_cast<char>((buffer >> bitsLeft) & 0xFF));
        }
    }
    return output;
}

QString formatCode(quint32 value, int digits) {
    QString code = QString::number(value);
    while (code.size() < digits) {
        code.prepend(QLatin1Char('0'));
    }
    if (code.size() > digits) {
        code = code.right(digits);
    }
    return code;
}

} // namespace

QString normalizeSecret(const QString& input) {
    QString cleaned = input.trimmed().toUpper();
    cleaned.remove(QRegularExpression(QStringLiteral("[^A-Z2-7]")));
    return cleaned;
}

bool isValidSecret(const QString& secret) {
    const QByteArray key = decodeBase32(secret);
    return key.size() >= 10;
}

QString currentCode(const QString& secret, int periodSeconds, int digits) {
    const QByteArray key = decodeBase32(secret);
    if (key.isEmpty() || periodSeconds <= 0 || digits <= 0) {
        return QString();
    }

    quint64 counter = static_cast<quint64>(
        QDateTime::currentDateTimeUtc().toSecsSinceEpoch() / periodSeconds);

    unsigned char counterBytes[8];
    for (int i = 7; i >= 0; --i) {
        counterBytes[i] = static_cast<unsigned char>(counter & 0xFF);
        counter >>= 8;
    }

    crypto_auth_hmacsha256_state state;
    if (crypto_auth_hmacsha256_init(
            &state,
            reinterpret_cast<const unsigned char*>(key.constData()),
            static_cast<unsigned long long>(key.size())) != 0) {
        return QString();
    }
    if (crypto_auth_hmacsha256_update(&state, counterBytes, sizeof(counterBytes)) != 0) {
        return QString();
    }
    unsigned char hmac[32];
    if (crypto_auth_hmacsha256_final(&state, hmac) != 0) {
        return QString();
    }

    const int offset = hmac[31] & 0x0F;
    const quint32 binary =
        ((static_cast<quint32>(hmac[offset]) & 0x7F) << 24)
        | (static_cast<quint32>(hmac[offset + 1]) << 16)
        | (static_cast<quint32>(hmac[offset + 2]) << 8)
        | static_cast<quint32>(hmac[offset + 3]);

    const quint32 mod = static_cast<quint32>(1);
    quint32 divisor = mod;
    for (int i = 1; i < digits; ++i) {
        divisor *= 10;
    }
    return formatCode(binary % divisor, digits);
}

int secondsRemaining(int periodSeconds) {
    if (periodSeconds <= 0) {
        return 0;
    }
    const qint64 now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    return static_cast<int>(periodSeconds - (now % periodSeconds));
}

} // namespace TotpGenerator
