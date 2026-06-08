#pragma once

#include <QObject>
#include <QJsonArray>
#include <QString>
#include <QVariantMap>
#include <QVector>

#include "models/CredentialSummary.h"
#include "models/Note.h"
#include "storage/Database.h"
#include "storage/VaultRepository.h"
#include "storage/NoteRepository.h"
#include "storage/CredentialRepository.h"
#include "security/VaultSession.h"

class GrimVaultController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool unlocked READ isUnlocked NOTIFY unlockedChanged)
    Q_PROPERTY(bool vaultExists READ vaultExists NOTIFY vaultExistsChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QVariantList notes READ noteSummaries NOTIFY notesChanged)
    Q_PROPERTY(QVariantList credentials READ credentialSummaries NOTIFY credentialsChanged)

public:
    explicit GrimVaultController(QObject* parent = nullptr);

    bool isUnlocked() const { return m_session.isUnlocked(); }
    bool vaultExists() const;
    QString accentColor() const { return m_accent; }
    void setAccentColor(const QString& hex);

    QJsonArray credentialsForOrigin(const QString& origin) const;
    QString credentialField(const QString& credentialId, const QString& field) const;

    Q_INVOKABLE bool unlock(const QString& password);
    Q_INVOKABLE bool createVault(const QString& password);
    Q_INVOKABLE void lock();
    Q_INVOKABLE bool biometricUnlock();
    Q_INVOKABLE bool biometricSupported() const;
    Q_INVOKABLE bool biometricConfigured() const;
    Q_INVOKABLE bool enableBiometric(const QString& password);
    Q_INVOKABLE void disableBiometric();

    Q_INVOKABLE QVariantList noteSummaries() const;
    Q_INVOKABLE QString noteBody(qint64 noteId) const;
    Q_INVOKABLE bool saveNote(qint64 noteId, const QString& title, const QString& body);
    Q_INVOKABLE qint64 createNote(const QString& title);

    Q_INVOKABLE QVariantList credentialSummaries() const;
    Q_INVOKABLE QString credentialPassword(qint64 id) const;
    Q_INVOKABLE QVariantMap credentialDetails(qint64 id) const;
    Q_INVOKABLE qint64 createCredential(
        const QString& label,
        const QString& username,
        const QString& password,
        const QString& url);
    Q_INVOKABLE bool updateCredential(
        qint64 id,
        const QString& label,
        const QString& username,
        const QString& password,
        const QString& url);
    Q_INVOKABLE bool deleteCredential(qint64 id);

    Q_INVOKABLE void saveSettings(bool lineNumbers, bool wordWrap, bool autoLock, int autoLockMin);
    Q_INVOKABLE void resetSettings();
    Q_INVOKABLE bool lineNumbers() const;
    Q_INVOKABLE bool wordWrap() const;

signals:
    void unlockedChanged();
    void vaultExistsChanged();
    void accentColorChanged();
    void notesChanged();
    void credentialsChanged();
    void errorOccurred(const QString& message);

private:
    Database m_db;
    VaultSession m_session;
    VaultRepository m_vault;
    NoteRepository m_notes;
    CredentialRepository m_credentials;
    QString m_accent = QStringLiteral("#cc2200");
};
