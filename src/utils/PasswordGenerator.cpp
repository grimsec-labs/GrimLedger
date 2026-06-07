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
    const int alphabetSize = static_cast<int>(sizeof(alphabet) - 1);
    const int maxUnbiased = 256 - (256 % alphabetSize);

    QString result;
    result.reserve(length);

    QByteArray buf;
    buf.resize(64);
    int offset = 64;

    while (result.size() < length) {
        if (offset >= buf.size()) {
            randombytes_buf(buf.data(), static_cast<size_t>(buf.size()));
            offset = 0;
        }

        const unsigned char value = static_cast<unsigned char>(buf[offset++]);
        if (value >= maxUnbiased) {
            continue;
        }
        const int idx = value % alphabetSize;
        result.append(QChar::fromLatin1(alphabet[idx]));
    }

    sodium_memzero(buf.data(), static_cast<size_t>(buf.size()));
    return result;
}

} // namespace PasswordGenerator
