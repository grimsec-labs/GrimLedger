#include "GrimVaultController.h"

#if defined(Q_OS_ANDROID)
#include "android_jni_bridge.h"
#endif
#include "bridge/OriginMatcher.h"
#include "models/Credential.h"
#include "security/CryptoManager.h"
#include "security/PasswordManager.h"
#include "security/PlatformBiometricUnlock.h"
#include "utils/AppSettings.h"

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
    , m_session(this)
    , m_vault(m_db)
    , m_notes(m_db)
    , m_credentials(m_db) {
    m_db.open(Database::defaultVaultPath());
    m_accent = savedAccent();
#if defined(Q_OS_ANDROID)
    // JNI registration happens from main() after QML loads.
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
    emit accentColorChanged();
}

bool GrimVaultController::unlock(const QString& password) {
    QByteArray key;
    if (!m_vault.unlockVault(password, key)) {
        emit errorOccurred(QStringLiteral("Incorrect master password."));
        return false;
    }
    m_session.setKey(std::move(key));
    emit unlockedChanged();
    emit notesChanged();
    emit credentialsChanged();
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
    emit notesChanged();
    emit credentialsChanged();
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
    emit unlockedChanged();
    emit notesChanged();
    emit credentialsChanged();
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
    }
    return ok;
}

void GrimVaultController::disableBiometric() {
    PlatformBiometricUnlock::disable();
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
    const bool ok = m_notes.updateNote(note, m_session.key());
    if (ok) {
        emit notesChanged();
    }
    return ok;
}

qint64 GrimVaultController::createNote(const QString& title) {
    if (!isUnlocked()) {
        return 0;
    }
    Note note;
    note.title = title;
    const qint64 id = m_notes.createNote(note, m_session.key());
    if (id > 0) {
        emit notesChanged();
    }
    return id;
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

QVariantMap GrimVaultController::credentialDetails(qint64 id) const {
    QVariantMap row;
    if (!isUnlocked()) {
        return row;
    }
    const auto cred = m_credentials.getCredential(id, m_session.key());
    if (!cred) {
        return row;
    }
    row.insert(QStringLiteral("id"), cred->id);
    row.insert(QStringLiteral("label"), cred->label);
    row.insert(QStringLiteral("username"), cred->username);
    row.insert(QStringLiteral("password"), cred->password);
    row.insert(QStringLiteral("url"), cred->url);
    return row;
}

qint64 GrimVaultController::createCredential(
    const QString& label,
    const QString& username,
    const QString& password,
    const QString& url) {
    if (!isUnlocked()) {
        return 0;
    }
    Credential cred;
    cred.label = label.trimmed().isEmpty() ? QStringLiteral("Untitled") : label.trimmed();
    cred.username = username;
    cred.password = password;
    cred.url = url;
    const qint64 id = m_credentials.createCredential(cred, m_session.key());
    if (id > 0) {
        emit credentialsChanged();
    }
    return id;
}

bool GrimVaultController::updateCredential(
    qint64 id,
    const QString& label,
    const QString& username,
    const QString& password,
    const QString& url) {
    if (!isUnlocked()) {
        return false;
    }
    const auto existing = m_credentials.getCredential(id, m_session.key());
    if (!existing) {
        return false;
    }
    Credential cred = *existing;
    cred.label = label.trimmed().isEmpty() ? QStringLiteral("Untitled") : label.trimmed();
    cred.username = username;
    cred.password = password;
    cred.url = url;
    const bool ok = m_credentials.updateCredential(cred, m_session.key());
    if (ok) {
        emit credentialsChanged();
    }
    return ok;
}

bool GrimVaultController::deleteCredential(qint64 id) {
    if (!isUnlocked()) {
        return false;
    }
    const bool ok = m_credentials.deleteCredential(id);
    if (ok) {
        emit credentialsChanged();
    }
    return ok;
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

void GrimVaultController::saveSettings(bool lineNumbers, bool wordWrap, bool autoLock, int autoLockMin) {
    saveAccent(m_accent);
    AppSettings::setLineNumbersEnabled(lineNumbers);
    AppSettings::setWordWrapEnabled(wordWrap);
    AppSettings::setAutoLockEnabled(autoLock);
    AppSettings::setAutoLockMinutes(autoLockMin);
    AppSettings::sync();
}

void GrimVaultController::resetSettings() {
    AppSettings::resetToDefaults();
    saveAccent(QString::fromLatin1(kDefaultAccent));
    m_accent = QString::fromLatin1(kDefaultAccent);
    emit accentColorChanged();
}

bool GrimVaultController::lineNumbers() const {
    return AppSettings::lineNumbersEnabled();
}

bool GrimVaultController::wordWrap() const {
    return AppSettings::wordWrapEnabled();
}
