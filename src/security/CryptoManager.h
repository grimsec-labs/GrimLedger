#pragma once



#include <QByteArray>

#include <QString>

#include <optional>

#include <vector>



// Cryptographic operations via libsodium.

// Legacy fields use XSalsa20-Poly1305 (crypto_secretbox).

// New domain-bound fields use XChaCha20-Poly1305 AEAD (crypto_aead_xchacha20poly1305_ietf).

// Never log keys, passwords, or plaintext.

class CryptoManager {

public:

    static constexpr int kKeySize = 32;

    static constexpr int kSaltSize = 16;

    static constexpr int kNonceSize = 24;

    static constexpr unsigned char kFieldFormatV2 = 0x02;



    struct KdfParams {

        unsigned long long opsLimit = 0;

        size_t memLimit = 0;

    };



    static KdfParams defaultKdfParams();

    static bool isValidKdfParams(const KdfParams& params);

    static bool isValidSalt(const QByteArray& salt);



    static QByteArray randomBytes(int size);



    static std::optional<QByteArray> deriveKey(

        const QString& password,

        const QByteArray& salt,

        const KdfParams& params);



    // Domain-bound field encryption (v2). aad binds ciphertext to purpose/record.

    static std::optional<QByteArray> encryptField(

        const QByteArray& plaintext,

        const QByteArray& key,

        const QByteArray& associatedData);



    static std::optional<QByteArray> decryptField(

        const QByteArray& ciphertextWithNonce,

        const QByteArray& key,

        const QByteArray& associatedData);



    static bool isFormatV2(const QByteArray& blob);



    // Legacy secretbox — used only for migration reads and GRIMBKUP1.

    static std::optional<QByteArray> encryptLegacy(

        const QByteArray& plaintext,

        const QByteArray& key);

    static std::optional<QByteArray> decryptLegacy(

        const QByteArray& ciphertextWithNonce,

        const QByteArray& key);



    // AEAD for standalone envelopes (backups).

    static std::optional<QByteArray> encryptAead(

        const QByteArray& plaintext,

        const QByteArray& key,

        const QByteArray& associatedData);

    static std::optional<QByteArray> decryptAead(

        const QByteArray& ciphertextWithNonce,

        const QByteArray& key,

        const QByteArray& associatedData);



    static QByteArray verifyAssociatedData();

    static QByteArray noteTitleAssociatedData(qint64 noteId);

    static QByteArray noteBodyAssociatedData(qint64 noteId);

    static QByteArray attachmentAssociatedData(const QString& attachmentId);

    static QByteArray credentialFieldAssociatedData(qint64 credentialId, const char* field);

    static QByteArray credentialFillPolicyAssociatedData(qint64 credentialId);
    static QByteArray sealedBlockAssociatedData(qint64 noteId, const QString& blockId);
    static QByteArray chronicleEntryAssociatedData(qint64 entryId);
    static QByteArray grimShareAssociatedData(const QByteArray& header);
    static QByteArray runbookSessionAssociatedData(qint64 sessionId);

    static QByteArray backupEnvelopeAssociatedData(const QByteArray& header);
    static QByteArray deriveChamberKey(const QByteArray& masterKey, int chamberId);



    static void secureZero(QByteArray& data);

    static void secureZero(std::vector<unsigned char>& data);



    // Deprecated aliases — migrate callers to encryptField/decryptField.

    static std::optional<QByteArray> encrypt(

        const QByteArray& plaintext,

        const QByteArray& key) {

        return encryptLegacy(plaintext, key);

    }

    static std::optional<QByteArray> decrypt(

        const QByteArray& ciphertextWithNonce,

        const QByteArray& key) {

        return decryptLegacy(ciphertextWithNonce, key);

    }

};


