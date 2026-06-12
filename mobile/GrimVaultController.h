#pragma once

#include <QObject>
#include <QJsonArray>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVector>

#include "models/CredentialSummary.h"
#include "models/Note.h"
#include "storage/Database.h"
#include "storage/VaultRepository.h"
#include "storage/NoteRepository.h"
#include "storage/CredentialRepository.h"
#include "storage/AttachmentRepository.h"
#include "security/VaultSession.h"

class GrimVaultController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool unlocked READ isUnlocked NOTIFY unlockedChanged)
    Q_PROPERTY(bool vaultExists READ vaultExists NOTIFY vaultExistsChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(bool biometricSupported READ biometricSupported CONSTANT)
    Q_PROPERTY(bool biometricConfigured READ biometricConfigured NOTIFY biometricConfiguredChanged)

public:
    explicit GrimVaultController(QObject* parent = nullptr);

    bool isUnlocked() const { return m_session.isUnlocked(); }
    bool vaultExists() const;
    QString accentColor() const { return m_accent; }
    void setAccentColor(const QString& hex);
    bool biometricSupported() const;
    bool biometricConfigured() const;

    QJsonArray credentialsForOrigin(const QString& origin) const;
    QString credentialField(const QString& credentialId, const QString& field) const;

    Q_INVOKABLE bool unlock(const QString& password);
    Q_INVOKABLE bool createVault(const QString& password);
    Q_INVOKABLE void lock();
    Q_INVOKABLE bool biometricUnlock();
    Q_INVOKABLE bool enableBiometric(const QString& password);
    Q_INVOKABLE void disableBiometric();

    // Notes
    Q_INVOKABLE QVariantList noteSummaries() const;
    Q_INVOKABLE QVariantList noteSummariesFiltered(qint64 folderId, bool favoritesOnly,
                                                      int sortField = 2, bool descending = true) const;
    Q_INVOKABLE QString noteBody(qint64 noteId) const;
    Q_INVOKABLE QVariantMap noteDetail(qint64 noteId) const;
    Q_INVOKABLE bool saveNote(qint64 noteId, const QString& title, const QString& body);
    Q_INVOKABLE bool saveNoteEx(qint64 noteId, const QString& title, const QString& body,
                                 qint64 folderId, bool isFavorite, const QStringList& tags);
    Q_INVOKABLE qint64 createNote(const QString& title);
    Q_INVOKABLE bool deleteNote(qint64 noteId);
    Q_INVOKABLE QVariantList searchNotes(const QString& query);

    // Multi-note operations
    Q_INVOKABLE int deleteNotes(const QVariantList& noteIds);
    Q_INVOKABLE int setNotesFavorite(const QVariantList& noteIds, bool favorite);
    Q_INVOKABLE int moveNotesToFolder(const QVariantList& noteIds, qint64 folderId);

    // Note export
    Q_INVOKABLE bool exportNoteMarkdown(qint64 noteId, const QUrl& destUrl);
    Q_INVOKABLE bool exportNotesMarkdown(const QVariantList& noteIds, const QUrl& destUrl);
    Q_INVOKABLE bool exportNoteHtml(qint64 noteId, const QUrl& destUrl);

    // Folders
    Q_INVOKABLE QVariantList folders() const;
    Q_INVOKABLE qint64 createFolder(const QString& name);
    Q_INVOKABLE bool renameFolder(qint64 id, const QString& name);
    Q_INVOKABLE bool deleteFolder(qint64 id);
    Q_INVOKABLE bool deleteFolderSafe(qint64 folderId);

    // Tags
    Q_INVOKABLE QStringList allTags() const;

    // Image attachments
    Q_INVOKABLE void insertImageIntoNote(qint64 noteId, const QUrl& fileUrl);
    Q_INVOKABLE void insertCameraImageIntoNote(qint64 noteId, const QUrl& captureUrl);
    Q_INVOKABLE void launchCamera(qint64 noteId);
    Q_INVOKABLE QString attachmentPreviewUrl(qint64 noteId, const QString& attachmentId) const;
    Q_INVOKABLE QVariantList noteImagePreviews(qint64 noteId) const;
    Q_INVOKABLE QVariantList allImageAttachments() const;
    Q_INVOKABLE bool deleteAttachment(const QString& attachmentId, qint64 noteId);
    Q_INVOKABLE bool exportAttachment(const QString& attachmentId, qint64 noteId, const QUrl& destUrl);

    // Credentials
    Q_INVOKABLE QVariantList credentialSummaries() const;
    Q_INVOKABLE QString credentialPassword(qint64 id) const;
    Q_INVOKABLE QVariantMap credentialDetail(qint64 id) const;
    Q_INVOKABLE qint64 createCredential(const QString& label, const QString& username,
                                         const QString& password, const QString& url,
                                         const QString& notes, const QString& totpSecret);
    Q_INVOKABLE qint64 createCredentialEx(const QString& label, const QString& username,
                                           const QString& password, const QString& url,
                                           const QString& notes, const QString& totpSecret,
                                           int fillTrustLevel);
    Q_INVOKABLE bool updateCredential(qint64 id, const QString& label, const QString& username,
                                       const QString& password, const QString& url,
                                       const QString& notes, const QString& totpSecret);
    Q_INVOKABLE bool updateCredentialEx(qint64 id, const QString& label, const QString& username,
                                         const QString& password, const QString& url,
                                         const QString& notes, const QString& totpSecret,
                                         int fillTrustLevel);
    Q_INVOKABLE bool deleteCredential(qint64 id);
    Q_INVOKABLE QVariantList searchCredentials(const QString& query);
    Q_INVOKABLE QString totpCode(qint64 id);
    Q_INVOKABLE int totpSecondsRemaining();

    // Password generator
    Q_INVOKABLE QString generatePassword(int length, bool upper, bool lower,
                                          bool digits, bool symbols, bool avoidAmbiguous);

    // Fill trust level labels for UI
    Q_INVOKABLE QVariantList fillTrustLevels() const;

    Q_INVOKABLE void copyToClipboard(const QString& text);

    // Camera permission
    Q_INVOKABLE int cameraPermissionStatus() const;
    Q_INVOKABLE void requestCameraPermission();
    Q_INVOKABLE void openAppSettings();

    Q_INVOKABLE void saveSettings(bool lineNumbers, bool wordWrap, bool autoLock, int autoLockMin);
    Q_INVOKABLE void resetSettings();
    Q_INVOKABLE bool lineNumbers() const;
    Q_INVOKABLE bool wordWrap() const;

signals:
    void unlockedChanged();
    void vaultExistsChanged();
    void accentColorChanged();
    void biometricConfiguredChanged();
    void imageAttached(qint64 noteId, const QString& markdown, const QString& previewDataUrl);
    void attachmentDeleted(const QString& attachmentId, qint64 noteId);
    void exportFinished(const QString& message);
    void errorOccurred(const QString& message);
    void cameraPermissionNeeded();
    void cameraPermissionResult(bool granted);

private:
    Database m_db;
    VaultSession m_session;
    VaultRepository m_vault;
    NoteRepository m_notes;
    CredentialRepository m_credentials;
    AttachmentRepository m_attachments;
    QString m_accent = QStringLiteral("#cc2200");
    qint64 m_pendingCameraNoteId = 0;

    void launchCameraInternal();

public:
    void onCameraResult(const QString& filePath);
};
