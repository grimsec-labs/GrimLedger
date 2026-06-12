#include "GrimVaultController.h"

#if defined(Q_OS_ANDROID)
#include "android_jni_bridge.h"
#include <QJniObject>
#include <QJniEnvironment>
#include <QCoreApplication>
#endif
#include "bridge/OriginMatcher.h"
#include "models/Credential.h"
#include "models/FillTrustLevel.h"
#include "search/SearchEngine.h"
#include "security/CryptoManager.h"
#include "security/PasswordManager.h"
#include "security/PlatformBiometricUnlock.h"
#include "utils/AppSettings.h"
#include "utils/ImageSanitizer.h"
#include "utils/PasswordGenerator.h"
#include "utils/TotpGenerator.h"

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QJsonObject>
#include <QSettings>
#include <QVariantMap>

#include <sodium.h>

namespace {
constexpr auto kDefaultAccent = "#cc2200";
QString savedAccent() {
    QSettings settings;
    return settings.value(QStringLiteral("appearance/accent"), QLatin1StringView(kDefaultAccent)).toString();
}
void saveAccent(const QString& hex) {
    QSettings settings;
    settings.setValue(QStringLiteral("appearance/accent"), hex);
}
} // namespace

GrimVaultController::GrimVaultController(QObject* parent)
    : QObject(parent)
    , m_vault(m_db)
    , m_notes(m_db)
    , m_credentials(m_db)
    , m_attachments(m_db) {
    if (!m_db.open(Database::defaultVaultPath())) {
        qWarning("GrimVaultController: failed to open vault database at %s",
                 qPrintable(Database::defaultVaultPath()));
    }
    m_accent = savedAccent();

    connect(&m_session, &VaultSession::lockRequested, this, &GrimVaultController::lock);

#if defined(Q_OS_ANDROID)
    AndroidJniBridge_registerController(this);
#endif
}

bool GrimVaultController::vaultExists() const {
    return m_vault.vaultExists();
}

void GrimVaultController::setAccentColor(const QString& hex) {
    if (m_accent == hex) {
        return;
    }
    m_accent = hex;
    saveAccent(hex);
    emit accentColorChanged();
}

bool GrimVaultController::unlock(const QString& password) {
    QByteArray key;
    if (!m_vault.unlockVault(password, key)) {
        emit errorOccurred(QStringLiteral("Incorrect master password."));
        return false;
    }
    m_session.setKey(std::move(key));
    m_session.setAutoLockEnabled(AppSettings::autoLockEnabled());
    m_session.setAutoLockMinutes(AppSettings::autoLockMinutes());
    emit unlockedChanged();
    return true;
}

bool GrimVaultController::createVault(const QString& password) {
    QString err;
    if (!PasswordManager::isValidVaultPassword(password, &err)) {
        emit errorOccurred(err);
        return false;
    }
    if (!m_vault.createVault(password)) {
        emit errorOccurred(QStringLiteral("Could not create vault."));
        return false;
    }
    return unlock(password);
}

void GrimVaultController::lock() {
    m_session.lock();
    emit unlockedChanged();
}

bool GrimVaultController::biometricUnlock() {
    QByteArray key;
    if (!PlatformBiometricUnlock::tryUnlock(key)) {
        emit errorOccurred(PlatformBiometricUnlock::lastError());
        return false;
    }
    if (!m_vault.unlockWithDerivedKey(key)) {
        PlatformBiometricUnlock::disable();
        CryptoManager::secureZero(key);
        emit errorOccurred(QStringLiteral("Stored biometric unlock is no longer valid."));
        return false;
    }
    m_session.setKey(std::move(key));
    m_session.setAutoLockEnabled(AppSettings::autoLockEnabled());
    m_session.setAutoLockMinutes(AppSettings::autoLockMinutes());
    emit unlockedChanged();
    return true;
}

bool GrimVaultController::biometricSupported() const {
    return PlatformBiometricUnlock::isPlatformSupported();
}

bool GrimVaultController::biometricConfigured() const {
    return PlatformBiometricUnlock::isConfigured();
}

bool GrimVaultController::enableBiometric(const QString& password) {
    QByteArray verifyKey;
    if (!m_vault.unlockVault(password, verifyKey)) {
        emit errorOccurred(QStringLiteral("Master password verification failed."));
        return false;
    }
    const bool ok = PlatformBiometricUnlock::enable(verifyKey, password);
    CryptoManager::secureZero(verifyKey);
    if (!ok) {
        emit errorOccurred(PlatformBiometricUnlock::lastError());
        return false;
    }
    emit biometricConfiguredChanged();
    return true;
}

void GrimVaultController::disableBiometric() {
    PlatformBiometricUnlock::disable();
    emit biometricConfiguredChanged();
}

QVariantList GrimVaultController::noteSummaries() const {
    QVariantList out;
    if (!isUnlocked()) {
        return out;
    }
    for (const Note& n : m_notes.listNotes(m_session.key())) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), n.id);
        row.insert(QStringLiteral("title"), n.title);
        out.append(row);
    }
    return out;
}

QString GrimVaultController::noteBody(qint64 noteId) const {
    if (!isUnlocked()) {
        return {};
    }
    const auto note = m_notes.getNote(noteId, m_session.key());
    return note ? note->body : QString();
}

bool GrimVaultController::saveNote(qint64 noteId, const QString& title, const QString& body) {
    if (!isUnlocked()) {
        return false;
    }
    Note note;
    note.id = noteId;
    note.title = title;
    note.body = body;
    return m_notes.updateNote(note, m_session.key());
}

qint64 GrimVaultController::createNote(const QString& title) {
    if (!isUnlocked()) {
        return 0;
    }
    Note note;
    note.title = title;
    return m_notes.createNote(note, m_session.key());
}

QVariantList GrimVaultController::credentialSummaries() const {
    QVariantList out;
    if (!isUnlocked()) {
        return out;
    }
    for (const CredentialSummary& c : m_credentials.listCredentialSummaries(m_session.key())) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), c.id);
        row.insert(QStringLiteral("label"), c.label);
        row.insert(QStringLiteral("username"), c.username);
        row.insert(QStringLiteral("url"), c.url);
        out.append(row);
    }
    return out;
}

QString GrimVaultController::credentialPassword(qint64 id) const {
    if (!isUnlocked()) {
        return {};
    }
    const auto cred = m_credentials.getCredential(id, m_session.key());
    return cred ? cred->password : QString();
}

QJsonArray GrimVaultController::credentialsForOrigin(const QString& origin) const {
    QJsonArray out;
    if (!isUnlocked()) {
        return out;
    }
    for (const CredentialSummary& c : m_credentials.listCredentialSummaries(m_session.key())) {
        if (!OriginMatcher::pageOriginMatchesCredentialUrl(origin, c.url, c.fillTrustLevel)) {
            continue;
        }
        QJsonObject row;
        row.insert(QStringLiteral("id"), QString::number(c.id));
        row.insert(QStringLiteral("label"), c.label);
        row.insert(QStringLiteral("username"), c.username);
        out.append(row);
    }
    return out;
}

QString GrimVaultController::credentialField(const QString& credentialId, const QString& field) const {
    if (!isUnlocked()) {
        return {};
    }
    const auto cred = m_credentials.getCredential(credentialId.toLongLong(), m_session.key());
    if (!cred) {
        return {};
    }
    if (field == QStringLiteral("username")) {
        return cred->username;
    }
    if (field == QStringLiteral("password")) {
        return cred->password;
    }
    return {};
}

bool GrimVaultController::deleteNote(qint64 noteId) {
    if (!isUnlocked()) {
        return false;
    }
    return m_notes.deleteNote(noteId);
}

QVariantList GrimVaultController::noteSummariesFiltered(qint64 folderId, bool favoritesOnly) const {
    QVariantList out;
    if (!isUnlocked()) {
        return out;
    }
    for (const Note& n : m_notes.listNotes(m_session.key(), NoteSortField::Modified, true, folderId, favoritesOnly)) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), n.id);
        row.insert(QStringLiteral("title"), n.title);
        row.insert(QStringLiteral("isFavorite"), n.isFavorite);
        row.insert(QStringLiteral("folderId"), n.folderId);
        row.insert(QStringLiteral("folderName"), n.folderName);
        row.insert(QStringLiteral("tags"), QStringList(n.tags.begin(), n.tags.end()));
        out.append(row);
    }
    return out;
}

QVariantMap GrimVaultController::noteDetail(qint64 noteId) const {
    QVariantMap out;
    if (!isUnlocked() || noteId <= 0) {
        return out;
    }
    const auto note = m_notes.getNote(noteId, m_session.key());
    if (!note) {
        return out;
    }
    out.insert(QStringLiteral("id"), note->id);
    out.insert(QStringLiteral("title"), note->title);
    out.insert(QStringLiteral("body"), note->body);
    out.insert(QStringLiteral("folderId"), note->folderId);
    out.insert(QStringLiteral("isFavorite"), note->isFavorite);
    out.insert(QStringLiteral("folderName"), note->folderName);
    out.insert(QStringLiteral("tags"), QStringList(note->tags.begin(), note->tags.end()));
    return out;
}

bool GrimVaultController::saveNoteEx(qint64 noteId, const QString& title, const QString& body,
                                      qint64 folderId, bool isFavorite, const QStringList& tags) {
    if (!isUnlocked()) {
        return false;
    }
    Note note;
    note.id = noteId;
    note.title = title;
    note.body = body;
    note.folderId = folderId;
    note.isFavorite = isFavorite;
    note.tags = QVector<QString>(tags.begin(), tags.end());
    return m_notes.updateNote(note, m_session.key());
}

QVariantList GrimVaultController::folders() const {
    QVariantList out;
    for (const Folder& f : m_notes.listFolders()) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), f.id);
        row.insert(QStringLiteral("name"), f.name);
        row.insert(QStringLiteral("parentId"), f.parentId);
        out.append(row);
    }
    return out;
}

qint64 GrimVaultController::createFolder(const QString& name) {
    return m_notes.createFolder(name);
}

bool GrimVaultController::renameFolder(qint64 id, const QString& name) {
    return m_notes.renameFolder(id, name);
}

bool GrimVaultController::deleteFolder(qint64 id) {
    return m_notes.deleteFolder(id);
}

QStringList GrimVaultController::allTags() const {
    QStringList out;
    for (const Tag& t : m_notes.listTags()) {
        out.append(t.name);
    }
    return out;
}

QVariantList GrimVaultController::searchNotes(const QString& query) {
    QVariantList out;
    if (!isUnlocked() || query.trimmed().isEmpty()) {
        return out;
    }
    const auto notes = m_notes.listNotes(m_session.key());
    QHash<qint64, QString> bodyMap;
    for (const Note& n : notes) {
        const auto full = m_notes.getNote(n.id, m_session.key());
        if (full) {
            bodyMap.insert(n.id, full->body);
        }
    }
    const auto matches = SearchEngine::search(notes, query, true, bodyMap);
    for (const SearchMatch& m : matches) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), m.note.id);
        row.insert(QStringLiteral("title"), m.note.title);
        row.insert(QStringLiteral("bodyMatch"), !m.bodyMatchPositions.isEmpty());
        out.append(row);
    }
    return out;
}

void GrimVaultController::insertImageIntoNote(qint64 noteId, const QUrl& fileUrl) {
    // Gate on unlocked vault: a locked vault has no session key and must never
    // accept new plaintext image bytes.
    if (!isUnlocked()) {
        emit errorOccurred(QStringLiteral("Unlock the vault before inserting an image."));
        return;
    }
    if (noteId <= 0) {
        emit errorOccurred(QStringLiteral("Open or create a note before inserting an image."));
        return;
    }
    if (fileUrl.isEmpty()) {
        return; // user cancelled the picker
    }

    // On Android the picker returns a content:// URI; on desktop a file:// URL.
    // QFile reads both on Qt 6. We never copy the original into app storage.
    const QString source = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    if (source.isEmpty()) {
        emit errorOccurred(QStringLiteral("Could not read the selected image."));
        return;
    }

    QByteArray original;
    {
        QFile file(source);
        if (!file.open(QIODevice::ReadOnly)) {
            emit errorOccurred(QStringLiteral("Could not open the selected image."));
            return;
        }
        // Read at most one byte past the cap so oversize input is rejected without
        // pulling an unbounded file into memory.
        original = file.read(ImageSanitizer::kMaxInputBytes + 1);
        file.close();
    }

    SanitizedImage sanitized;
    QString err;
    const bool ok = ImageSanitizer::sanitizeFromData(original, sanitized, &err);
    // Wipe the original (un-sanitized, metadata-bearing) bytes from memory ASAP.
    CryptoManager::secureZero(original);
    if (!ok) {
        emit errorOccurred(err.isEmpty() ? QStringLiteral("Unsupported or corrupt image.") : err);
        return;
    }

    // Generic original_name (not the picked filename) — the name column is stored
    // in the clear, so we avoid leaking a potentially sensitive filename into the DB.
    const QString attachmentId = m_attachments.storeAttachment(
        noteId,
        sanitized.pngData,
        QStringLiteral("image/png"),
        QStringLiteral("image"),
        m_session.key());
    if (attachmentId.isEmpty()) {
        CryptoManager::secureZero(sanitized.pngData);
        emit errorOccurred(QStringLiteral("Could not store image in the encrypted vault."));
        return;
    }

    // Build an in-memory preview from the sanitized PNG (already metadata-free) for
    // immediate confirmation. Never logged, never written to disk.
    const QString previewUrl = QStringLiteral("data:image/png;base64,")
        + QString::fromLatin1(sanitized.pngData.toBase64());
    CryptoManager::secureZero(sanitized.pngData);

    const QString markdown = QStringLiteral("![image](%1%2)\n")
        .arg(QString::fromUtf8(AttachmentRepository::kGrimScheme), attachmentId);
    emit imageAttached(noteId, markdown, previewUrl);
}

QString GrimVaultController::attachmentPreviewUrl(qint64 noteId, const QString& attachmentId) const {
    if (!isUnlocked() || noteId <= 0) {
        return {};
    }
    const auto data = m_attachments.loadAttachment(attachmentId, noteId, m_session.key());
    if (!data) {
        return {};
    }
    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(data->toBase64());
}

QVariantList GrimVaultController::noteImagePreviews(qint64 noteId) const {
    QVariantList out;
    if (!isUnlocked() || noteId <= 0) {
        return out;
    }
    const QString body = noteBody(noteId);
    const auto refs = AttachmentRepository::extractImageRefs(body);
    for (const auto& [url, id] : refs) {
        const auto data = m_attachments.loadAttachment(id, noteId, m_session.key());
        QVariantMap entry;
        entry.insert(QStringLiteral("attachmentId"), id);
        entry.insert(QStringLiteral("noteId"), noteId);
        if (data && !data->isEmpty()) {
            entry.insert(QStringLiteral("previewUrl"),
                         QStringLiteral("data:image/png;base64,") + QString::fromLatin1(data->toBase64()));
        } else {
            entry.insert(QStringLiteral("previewUrl"), QString());
        }
        out.append(entry);
    }
    return out;
}

QVariantList GrimVaultController::allImageAttachments() const {
    QVariantList out;
    if (!isUnlocked()) {
        return out;
    }

    QHash<qint64, QString> titleCache;
    for (const Note& n : m_notes.listNotes(m_session.key())) {
        titleCache.insert(n.id, n.title);
    }

    const auto attachments = m_attachments.listAllImageAttachments();
    for (const QVariantMap& a : attachments) {
        QVariantMap entry = a;
        const qint64 noteId = a.value(QStringLiteral("noteId")).toLongLong();
        entry.insert(QStringLiteral("noteTitle"), titleCache.value(noteId));
        const auto data = m_attachments.loadAttachment(
            a.value(QStringLiteral("id")).toString(),
            noteId,
            m_session.key());
        if (data && !data->isEmpty()) {
            entry.insert(QStringLiteral("previewUrl"),
                         QStringLiteral("data:image/png;base64,") + QString::fromLatin1(data->toBase64()));
        }
        out.append(entry);
    }
    return out;
}

bool GrimVaultController::deleteAttachment(const QString& attachmentId, qint64 noteId) {
    if (!isUnlocked()) {
        emit errorOccurred(QStringLiteral("Unlock the vault first."));
        return false;
    }
    if (!m_attachments.deleteAttachment(attachmentId, noteId)) {
        emit errorOccurred(QStringLiteral("Could not delete attachment."));
        return false;
    }
    emit attachmentDeleted(attachmentId, noteId);
    return true;
}

bool GrimVaultController::exportAttachment(const QString& attachmentId, qint64 noteId, const QUrl& destUrl) {
    if (!isUnlocked()) {
        emit errorOccurred(QStringLiteral("Unlock the vault before exporting."));
        return false;
    }
    if (attachmentId.isEmpty() || noteId <= 0 || destUrl.isEmpty()) {
        emit errorOccurred(QStringLiteral("Invalid export parameters."));
        return false;
    }

    const auto data = m_attachments.loadAttachment(attachmentId, noteId, m_session.key());
    if (!data || data->isEmpty()) {
        emit errorOccurred(QStringLiteral("Attachment not found or could not be decrypted."));
        return false;
    }

    const QString dest = destUrl.isLocalFile() ? destUrl.toLocalFile() : destUrl.toString();
    QFile file(dest);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(QStringLiteral("Could not write to the selected location."));
        return false;
    }
    file.write(*data);
    file.close();

    emit exportFinished(QStringLiteral("Image exported successfully."));
    return true;
}

void GrimVaultController::insertCameraImageIntoNote(qint64 noteId, const QUrl& captureUrl) {
    insertImageIntoNote(noteId, captureUrl);
}

void GrimVaultController::launchCamera(qint64 noteId) {
#if defined(Q_OS_ANDROID)
    if (!isUnlocked()) {
        emit errorOccurred(QStringLiteral("Unlock the vault before using the camera."));
        return;
    }
    if (noteId <= 0) {
        emit errorOccurred(QStringLiteral("Open or create a note before capturing an image."));
        return;
    }
    m_pendingCameraNoteId = noteId;
    {
        QSettings s;
        s.setValue(QStringLiteral("camera/pendingNoteId"), noteId);
    }

    QJniObject context(QNativeInterface::QAndroidApplication::context());
    if (!context.isValid()) {
        emit errorOccurred(QStringLiteral("Failed to get Android context."));
        return;
    }

    QJniEnvironment env;
    QJniObject::callStaticMethod<void>(
        "org/grimseclabs/grimledger/GrimCameraActivity",
        "launch",
        "(Landroid/content/Context;)V",
        context.object());
    if (env.checkAndClearExceptions()) {
        emit errorOccurred(QStringLiteral("Failed to launch camera."));
    }
#else
    Q_UNUSED(noteId)
    emit errorOccurred(QStringLiteral("Camera capture is only available on Android."));
#endif
}

void GrimVaultController::onCameraResult(const QString& filePath) {
#if defined(Q_OS_ANDROID)
    if (filePath.isEmpty()) {
        return;
    }
    qint64 noteId = m_pendingCameraNoteId;
    if (noteId <= 0) {
        QSettings s;
        noteId = s.value(QStringLiteral("camera/pendingNoteId"), 0).toLongLong();
    }
    m_pendingCameraNoteId = 0;
    {
        QSettings s;
        s.remove(QStringLiteral("camera/pendingNoteId"));
    }

    if (noteId <= 0) {
        QJniObject::callStaticMethod<void>(
            "org/grimseclabs/grimledger/GrimCameraActivity",
            "deleteTempFile",
            "(Ljava/lang/String;)V",
            QJniObject::fromString(filePath).object<jstring>());
        emit errorOccurred(QStringLiteral("No note selected for camera image."));
        return;
    }

    if (!isUnlocked()) {
        QJniObject::callStaticMethod<void>(
            "org/grimseclabs/grimledger/GrimCameraActivity",
            "deleteTempFile",
            "(Ljava/lang/String;)V",
            QJniObject::fromString(filePath).object<jstring>());
        emit errorOccurred(QStringLiteral("Vault locked — camera image discarded."));
        return;
    }

    insertImageIntoNote(noteId, QUrl::fromLocalFile(filePath));

    QJniObject::callStaticMethod<void>(
        "org/grimseclabs/grimledger/GrimCameraActivity",
        "deleteTempFile",
        "(Ljava/lang/String;)V",
        QJniObject::fromString(filePath).object<jstring>());
#else
    Q_UNUSED(filePath)
#endif
}

QVariantMap GrimVaultController::credentialDetail(qint64 id) const {
    QVariantMap out;
    if (!isUnlocked()) {
        return out;
    }
    const auto cred = m_credentials.getCredential(id, m_session.key());
    if (!cred) {
        return out;
    }
    out.insert(QStringLiteral("id"), cred->id);
    out.insert(QStringLiteral("label"), cred->label);
    out.insert(QStringLiteral("username"), cred->username);
    out.insert(QStringLiteral("password"), cred->password);
    out.insert(QStringLiteral("url"), cred->url);
    out.insert(QStringLiteral("notes"), cred->notes);
    out.insert(QStringLiteral("totpSecret"), cred->totpSecret);
    out.insert(QStringLiteral("fillTrustLevel"), static_cast<int>(cred->fillTrustLevel));
    return out;
}

qint64 GrimVaultController::createCredential(const QString& label, const QString& username,
                                              const QString& password, const QString& url,
                                              const QString& notes, const QString& totpSecret) {
    return createCredentialEx(label, username, password, url, notes, totpSecret,
                              static_cast<int>(FillTrustLevel::ExactOrigin));
}

qint64 GrimVaultController::createCredentialEx(const QString& label, const QString& username,
                                                const QString& password, const QString& url,
                                                const QString& notes, const QString& totpSecret,
                                                int fillTrustLevel) {
    if (!isUnlocked()) {
        return 0;
    }
    Credential cred;
    cred.label = label;
    cred.username = username;
    cred.password = password;
    cred.url = url;
    cred.notes = notes;
    cred.totpSecret = totpSecret;
    cred.fillTrustLevel = fillTrustLevelFromInt(fillTrustLevel);
    return m_credentials.createCredential(cred, m_session.key());
}

bool GrimVaultController::updateCredential(qint64 id, const QString& label, const QString& username,
                                            const QString& password, const QString& url,
                                            const QString& notes, const QString& totpSecret) {
    return updateCredentialEx(id, label, username, password, url, notes, totpSecret,
                              static_cast<int>(FillTrustLevel::ExactOrigin));
}

bool GrimVaultController::updateCredentialEx(qint64 id, const QString& label, const QString& username,
                                              const QString& password, const QString& url,
                                              const QString& notes, const QString& totpSecret,
                                              int fillTrustLevel) {
    if (!isUnlocked()) {
        return false;
    }
    Credential cred;
    cred.id = id;
    cred.label = label;
    cred.username = username;
    cred.password = password;
    cred.url = url;
    cred.notes = notes;
    cred.totpSecret = totpSecret;
    cred.fillTrustLevel = fillTrustLevelFromInt(fillTrustLevel);
    return m_credentials.updateCredential(cred, m_session.key());
}

bool GrimVaultController::deleteCredential(qint64 id) {
    if (!isUnlocked()) {
        return false;
    }
    return m_credentials.deleteCredential(id);
}

QVariantList GrimVaultController::searchCredentials(const QString& query) {
    QVariantList out;
    if (!isUnlocked() || query.trimmed().isEmpty()) {
        return out;
    }
    for (const CredentialSummary& c : m_credentials.searchCredentialSummaries(query, m_session.key())) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), c.id);
        row.insert(QStringLiteral("label"), c.label);
        row.insert(QStringLiteral("username"), c.username);
        row.insert(QStringLiteral("url"), c.url);
        out.append(row);
    }
    return out;
}

QString GrimVaultController::totpCode(qint64 id) {
    if (!isUnlocked()) {
        return {};
    }
    const auto cred = m_credentials.getCredential(id, m_session.key());
    if (!cred || cred->totpSecret.isEmpty()) {
        return {};
    }
    return TotpGenerator::currentCode(cred->totpSecret);
}

int GrimVaultController::totpSecondsRemaining() {
    return TotpGenerator::secondsRemaining();
}

void GrimVaultController::copyToClipboard(const QString& text) {
    if (auto* clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(text);
    }
}

void GrimVaultController::saveSettings(bool lineNumbers, bool wordWrap, bool autoLock, int autoLockMin) {
    saveAccent(m_accent);
    AppSettings::setLineNumbersEnabled(lineNumbers);
    AppSettings::setWordWrapEnabled(wordWrap);
    AppSettings::setAutoLockEnabled(autoLock);
    AppSettings::setAutoLockMinutes(autoLockMin);
    AppSettings::sync();
    m_session.setAutoLockEnabled(autoLock);
    m_session.setAutoLockMinutes(autoLockMin);
}

void GrimVaultController::resetSettings() {
    AppSettings::resetToDefaults();
    saveAccent(QString::fromLatin1(kDefaultAccent));
    m_accent = QString::fromLatin1(kDefaultAccent);
    emit accentColorChanged();
    m_session.setAutoLockEnabled(true);
    m_session.setAutoLockMinutes(15);
}

bool GrimVaultController::lineNumbers() const {
    return AppSettings::lineNumbersEnabled();
}

bool GrimVaultController::wordWrap() const {
    return AppSettings::wordWrapEnabled();
}

QString GrimVaultController::generatePassword(int length, bool upper, bool lower,
                                               bool digits, bool symbols, bool avoidAmbiguous) {
    if (length < 8) length = 8;
    if (length > 128) length = 128;

    QByteArray alphabet;
    if (lower) {
        alphabet.append("abcdefghijklmnopqrstuvwxyz");
    }
    if (upper) {
        alphabet.append("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    }
    if (digits) {
        alphabet.append("0123456789");
    }
    if (symbols) {
        alphabet.append("!@#$%^&*()-_=+[]{}:,.?");
    }
    if (alphabet.isEmpty()) {
        alphabet.append("abcdefghijklmnopqrstuvwxyz");
    }

    if (avoidAmbiguous) {
        static const char ambiguous[] = "0O1lI|";
        for (const char c : ambiguous) {
            alphabet.remove(alphabet.indexOf(c), 1);
        }
    }

    const int alphabetSize = alphabet.size();
    if (alphabetSize == 0) {
        return PasswordGenerator::generate(length);
    }
    const int maxUnbiased = 256 - (256 % alphabetSize);

    QString result;
    result.reserve(length);

    QByteArray buf(64, Qt::Uninitialized);
    int offset = 64;

    while (result.size() < length) {
        if (offset >= buf.size()) {
            randombytes_buf(buf.data(), static_cast<size_t>(buf.size()));
            offset = 0;
        }
        const unsigned char value = static_cast<unsigned char>(buf[offset++]);
        if (value >= static_cast<unsigned int>(maxUnbiased)) {
            continue;
        }
        result.append(QChar::fromLatin1(alphabet.at(value % alphabetSize)));
    }

    sodium_memzero(buf.data(), static_cast<size_t>(buf.size()));
    return result;
}

QVariantList GrimVaultController::fillTrustLevels() const {
    QVariantList out;
    for (int i = 0; i <= 3; ++i) {
        QVariantMap entry;
        entry.insert(QStringLiteral("value"), i);
        entry.insert(QStringLiteral("label"), fillTrustLevelLabel(fillTrustLevelFromInt(i)));
        out.append(entry);
    }
    return out;
}
