#pragma once

#include <QByteArray>
#include <QString>
#include <optional>
#include <vector>

// Cryptographic operations via libsodium.
// Uses Argon2id for KDF and XChaCha20-Poly1305 for authenticated encryption.
// Never log keys, passwords, or plaintext.
class CryptoManager {
public:
    static constexpr int kKeySize = 32;
    static constexpr int kSaltSize = 16;
    static constexpr int kNonceSize = 24;

    struct KdfParams {
        unsigned long long opsLimit = 0;
        size_t memLimit = 0;
    };

    static KdfParams defaultKdfParams();

    // Generate cryptographically secure random bytes.
    static QByteArray randomBytes(int size);

    // Derive a 32-byte key from password + salt using Argon2id.
    static std::optional<QByteArray> deriveKey(
        const QString& password,
        const QByteArray& salt,
        const KdfParams& params);

    // Encrypt plaintext with XChaCha20-Poly1305. Returns nonce || ciphertext.
    static std::optional<QByteArray> encrypt(
        const QByteArray& plaintext,
        const QByteArray& key);

    // Decrypt nonce || ciphertext. Returns plaintext or nullopt on auth failure.
    static std::optional<QByteArray> decrypt(
        const QByteArray& ciphertextWithNonce,
        const QByteArray& key);

    // Securely zero a byte buffer.
    static void secureZero(QByteArray& data);
    static void secureZero(std::vector<unsigned char>& data);
};
