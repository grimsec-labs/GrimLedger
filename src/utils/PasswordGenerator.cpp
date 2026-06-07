#include "utils/PasswordGenerator.h"

#include <sodium.h>

namespace PasswordGenerator {

QString generate(int length) {
    if (length < 8) {
        length = 8;
    }
    if (length > 128) {
        length = 128;
    }

    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "!@#$%^&*()-_=+[]{}:,.?";

    QByteArray buf(length, '\0');
    randombytes_buf(buf.data(), static_cast<size_t>(length));

    QString result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        const unsigned char idx = static_cast<unsigned char>(buf[i]) % (sizeof(alphabet) - 1);
        result.append(QChar::fromLatin1(alphabet[idx]));
    }
    sodium_memzero(buf.data(), static_cast<size_t>(buf.size()));
    return result;
}

} // namespace PasswordGenerator
