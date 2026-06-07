#include "security/CryptoManager.h"

#include <sodium.h>

#include <cstring>

CryptoManager::KdfParams CryptoManager::defaultKdfParams() {
    KdfParams params;
    params.opsLimit = crypto_pwhash_OPSLIMIT_MODERATE;
    params.memLimit = crypto_pwhash_MEMLIMIT_MODERATE;
    return params;
}

QByteArray CryptoManager::randomBytes(int size) {
    QByteArray buf(size, '\0');
    randombytes_buf(buf.data(), static_cast<size_t>(size));
    return buf;
}

std::optional<QByteArray> CryptoManager::deriveKey(
    const QString& password,
    const QByteArray& salt,
    const KdfParams& params) {
    if (salt.size() != kSaltSize) {
        return std::nullopt;
    }

    const QByteArray passUtf8 = password.toUtf8();
    QByteArray key(kKeySize, '\0');

    if (crypto_pwhash(
            reinterpret_cast<unsigned char*>(key.data()),
            static_cast<unsigned long long>(kKeySize),
            passUtf8.constData(),
            static_cast<unsigned long long>(passUtf8.size()),
            reinterpret_cast<const unsigned char*>(salt.constData()),
            params.opsLimit,
            params.memLimit,
            crypto_pwhash_ALG_ARGON2ID13) != 0) {
        secureZero(key);
        return std::nullopt;
    }

    return key;
}

std::optional<QByteArray> CryptoManager::encrypt(
    const QByteArray& plaintext,
    const QByteArray& key) {
    if (key.size() != kKeySize) {
        return std::nullopt;
    }

    const QByteArray nonce = randomBytes(kNonceSize);
    QByteArray ciphertext(
        static_cast<int>(crypto_secretbox_MACBYTES) + plaintext.size(),
        '\0');

    if (crypto_secretbox_easy(
            reinterpret_cast<unsigned char*>(ciphertext.data()),
            reinterpret_cast<const unsigned char*>(plaintext.constData()),
            static_cast<unsigned long long>(plaintext.size()),
            reinterpret_cast<const unsigned char*>(nonce.constData()),
            reinterpret_cast<const unsigned char*>(key.constData())) != 0) {
        return std::nullopt;
    }

    return nonce + ciphertext;
}

std::optional<QByteArray> CryptoManager::decrypt(
    const QByteArray& ciphertextWithNonce,
    const QByteArray& key) {
    if (key.size() != kKeySize) {
        return std::nullopt;
    }
    if (ciphertextWithNonce.size() < kNonceSize + crypto_secretbox_MACBYTES) {
        return std::nullopt;
    }

    const auto* nonce = reinterpret_cast<const unsigned char*>(ciphertextWithNonce.constData());
    const auto* cipher = reinterpret_cast<const unsigned char*>(
        ciphertextWithNonce.constData() + kNonceSize);
    const auto cipherLen = static_cast<unsigned long long>(
        ciphertextWithNonce.size() - kNonceSize);

    QByteArray plaintext(static_cast<int>(cipherLen - crypto_secretbox_MACBYTES), '\0');

    if (crypto_secretbox_open_easy(
            reinterpret_cast<unsigned char*>(plaintext.data()),
            cipher,
            cipherLen,
            nonce,
            reinterpret_cast<const unsigned char*>(key.constData())) != 0) {
        secureZero(plaintext);
        return std::nullopt;
    }

    return plaintext;
}

void CryptoManager::secureZero(QByteArray& data) {
    if (!data.isEmpty()) {
        sodium_memzero(data.data(), static_cast<size_t>(data.size()));
    }
    data.clear();
}

void CryptoManager::secureZero(std::vector<unsigned char>& data) {
    if (!data.empty()) {
        sodium_memzero(data.data(), data.size());
    }
    data.clear();
}
