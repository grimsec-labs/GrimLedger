#include "GrimVaultController.h"

#if defined(Q_OS_ANDROID)
#include "android_jni_bridge.h"
#include <QJniObject>
#include <QCoreApplication>
#endif
#include "bridge/OriginMatcher.h"
#include "models/Credential.h"
#include "search/SearchEngine.h"
#include "security/CryptoManager.h"
#include "security/PasswordManager.h"
#include "security/PlatformBiometricUnlock.h"
#include "utils/AppSettings.h"
#include "utils/ImageSanitizer.h"
#include "utils/TotpGenerator.h"

#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QJsonObject>
#include <QSettings>
#include <QVariantMap>

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

QVariantList GrimVaultController::searchNotes(const QString& query) {
    QVariantList out;
    if (!isUnlocked() || query.trimmed().isEmpty()) {
        return out;
    }
    const auto notes = m_notes.listNotes(m_session.key());
    const auto matches = SearchEngine::search(notes, query, false);
    for (const SearchMatch& m : matches) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), m.note.id);
        row.insert(QStringLiteral("title"), m.note.title);
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

    QJniObject context(QNativeInterface::QAndroidApplication::context());
    QJniObject::callStaticMethod<void>(
        "org/grimseclabs/grimledger/GrimCameraActivity",
        "launch",
        "(Landroid/content/Context;)V",
        context.object());
#else
    Q_UNUSED(noteId)
    emit errorOccurred(QStringLiteral("Camera capture is only available on Android."));
#endif
}

void GrimVaultController::onCameraResult(const QString& filePath) {
#if defined(Q_OS_ANDROID)
    if (filePath.isEmpty() || m_pendingCameraNoteId <= 0) {
        m_pendingCameraNoteId = 0;
        return;
    }
    const qint64 noteId = m_pendingCameraNoteId;
    m_pendingCameraNoteId = 0;

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
    return out;
}

qint64 GrimVaultController::createCredential(const QString& label, const QString& username,
                                              const QString& password, const QString& url,
                                              const QString& notes, const QString& totpSecret) {
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
    return m_credentials.createCredential(cred, m_session.key());
}

bool GrimVaultController::updateCredential(qint64 id, const QString& label, const QString& username,
                                            const QString& password, const QString& url,
                                            const QString& notes, const QString& totpSecret) {
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
