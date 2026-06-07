#include "security/CryptoManager.h"

#include <sodium.h>

#include <cstring>

CryptoManager::KdfParams CryptoManager::defaultKdfParams() {
    KdfParams params;
    params.opsLimit = crypto_pwhash_OPSLIMIT_MODERATE;
    params.memLimit = crypto_pwhash_MEMLIMIT_MODERATE;
    return params;
}

bool CryptoManager::isValidKdfParams(const KdfParams& params) {
    if (params.opsLimit < crypto_pwhash_OPSLIMIT_INTERACTIVE
        || params.opsLimit > crypto_pwhash_OPSLIMIT_SENSITIVE) {
        return false;
    }
    if (params.memLimit < crypto_pwhash_MEMLIMIT_INTERACTIVE
        || params.memLimit > 256 * 1024 * 1024) {
        return false;
    }
    return true;
}

bool CryptoManager::isValidSalt(const QByteArray& salt) {
    return salt.size() == kSaltSize;
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
    if (!isValidSalt(salt) || !isValidKdfParams(params)) {
        return std::nullopt;
    }

    QByteArray passUtf8 = password.toUtf8();
    QByteArray key(kKeySize, '\0');

    const int rc = crypto_pwhash(
        reinterpret_cast<unsigned char*>(key.data()),
        static_cast<unsigned long long>(kKeySize),
        passUtf8.constData(),
        static_cast<unsigned long long>(passUtf8.size()),
        reinterpret_cast<const unsigned char*>(salt.constData()),
        params.opsLimit,
        params.memLimit,
        crypto_pwhash_ALG_ARGON2ID13);

    secureZero(passUtf8);

    if (rc != 0) {
        secureZero(key);
        return std::nullopt;
    }

    return key;
}

bool CryptoManager::isFormatV2(const QByteArray& blob) {
    return blob.size() >= 1 + kNonceSize + crypto_aead_xchacha20poly1305_ietf_ABYTES
        && static_cast<unsigned char>(blob[0]) == kFieldFormatV2;
}

std::optional<QByteArray> CryptoManager::encryptAead(
    const QByteArray& plaintext,
    const QByteArray& key,
    const QByteArray& associatedData) {
    if (key.size() != kKeySize) {
        return std::nullopt;
    }

    const QByteArray nonce = randomBytes(kNonceSize);
    QByteArray ciphertext(
        static_cast<int>(crypto_aead_xchacha20poly1305_ietf_ABYTES) + plaintext.size(),
        '\0');

    unsigned long long cipherLen = 0;
    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            reinterpret_cast<unsigned char*>(ciphertext.data()),
            &cipherLen,
            reinterpret_cast<const unsigned char*>(plaintext.constData()),
            static_cast<unsigned long long>(plaintext.size()),
            reinterpret_cast<const unsigned char*>(associatedData.constData()),
            static_cast<unsigned long long>(associatedData.size()),
            nullptr,
            reinterpret_cast<const unsigned char*>(nonce.constData()),
            reinterpret_cast<const unsigned char*>(key.constData())) != 0) {
        return std::nullopt;
    }

    ciphertext.resize(static_cast<int>(cipherLen));
    return nonce + ciphertext;
}

std::optional<QByteArray> CryptoManager::decryptAead(
    const QByteArray& ciphertextWithNonce,
    const QByteArray& key,
    const QByteArray& associatedData) {
    if (key.size() != kKeySize) {
        return std::nullopt;
    }
    if (ciphertextWithNonce.size()
        < kNonceSize + crypto_aead_xchacha20poly1305_ietf_ABYTES) {
        return std::nullopt;
    }

    const auto* nonce = reinterpret_cast<const unsigned char*>(ciphertextWithNonce.constData());
    const auto* cipher = reinterpret_cast<const unsigned char*>(
        ciphertextWithNonce.constData() + kNonceSize);
    const auto cipherLen = static_cast<unsigned long long>(
        ciphertextWithNonce.size() - kNonceSize);

    QByteArray plaintext(
        static_cast<int>(cipherLen - crypto_aead_xchacha20poly1305_ietf_ABYTES),
        '\0');
    unsigned long long plainLen = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            reinterpret_cast<unsigned char*>(plaintext.data()),
            &plainLen,
            nullptr,
            cipher,
            cipherLen,
            reinterpret_cast<const unsigned char*>(associatedData.constData()),
            static_cast<unsigned long long>(associatedData.size()),
            nonce,
            reinterpret_cast<const unsigned char*>(key.constData())) != 0) {
        secureZero(plaintext);
        return std::nullopt;
    }

    plaintext.resize(static_cast<int>(plainLen));
    return plaintext;
}

std::optional<QByteArray> CryptoManager::encryptField(
    const QByteArray& plaintext,
    const QByteArray& key,
    const QByteArray& associatedData) {
    const auto enc = encryptAead(plaintext, key, associatedData);
    if (!enc) {
        return std::nullopt;
    }
    QByteArray out;
    out.reserve(1 + enc->size());
    out.append(static_cast<char>(kFieldFormatV2));
    out.append(*enc);
    return out;
}

std::optional<QByteArray> CryptoManager::decryptField(
    const QByteArray& ciphertextWithNonce,
    const QByteArray& key,
    const QByteArray& associatedData) {
    if (isFormatV2(ciphertextWithNonce)) {
        return decryptAead(ciphertextWithNonce.mid(1), key, associatedData);
    }
    return decryptLegacy(ciphertextWithNonce, key);
}

std::optional<QByteArray> CryptoManager::encryptLegacy(
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

std::optional<QByteArray> CryptoManager::decryptLegacy(
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

QByteArray CryptoManager::verifyAssociatedData() {
    return QByteArray("grim:verify:v1");
}

QByteArray CryptoManager::noteTitleAssociatedData(qint64 noteId) {
    return QByteArray("grim:note:") + QByteArray::number(noteId) + QByteArray(":title");
}

QByteArray CryptoManager::noteBodyAssociatedData(qint64 noteId) {
    return QByteArray("grim:note:") + QByteArray::number(noteId) + QByteArray(":body");
}

QByteArray CryptoManager::attachmentAssociatedData(const QString& attachmentId) {
    return QStringLiteral("grim:attach:%1").arg(attachmentId).toUtf8();
}

QByteArray CryptoManager::credentialFieldAssociatedData(qint64 credentialId, const char* field) {
    return QByteArray("grim:cred:") + QByteArray::number(credentialId) + ':' + field;
}

QByteArray CryptoManager::credentialFillPolicyAssociatedData(qint64 credentialId) {
    return QByteArray("grim:cred:") + QByteArray::number(credentialId) + QByteArray(":fill_policy");
}

QByteArray CryptoManager::backupEnvelopeAssociatedData(const QByteArray& header) {
    return QByteArray("grim:backup:v2:") + header;
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
