#include "App.h"
#include "ui/LoginWindow.h"
#include "ui/MainWindow.h"
#include "storage/Database.h"
#include "storage/VaultRepository.h"
#include "security/VaultSession.h"
#include "security/CryptoManager.h"
#include "utils/DialogUtils.h"
#include "utils/AppSettings.h"

#include <QFile>

App::App(QObject* parent)
    : QObject(parent)
    , m_db(std::make_unique<Database>())
    , m_session(std::make_unique<VaultSession>()) {}

App::~App() = default;

void App::start() {
    if (!openDatabase()) {
        DialogUtils::critical(
            nullptr,
            QStringLiteral("GrimLedger"),
            QStringLiteral("Could not initialize the vault database."));
        return;
    }

    m_vault = std::make_unique<VaultRepository>(*m_db);
    showLogin();
}

bool App::openDatabase() {
    const QString path = Database::defaultVaultPath();
    return m_db->open(path);
}

void App::showLogin() {
    if (!m_login) {
        m_login = new LoginWindow();
        connect(m_login, &LoginWindow::unlockRequested, this, &App::onUnlockRequested);
        connect(m_login, &LoginWindow::createVaultRequested, this, &App::onCreateVaultRequested);
    }

    AppSettings::resetFailedUnlockAttempts();
    m_login->setVaultExists(m_vault->vaultExists());
    m_login->clearPassword();
    m_login->show();
    m_login->raise();
    m_login->activateWindow();

    if (m_main) {
        m_main->hide();
    }
}

void App::showMain() {
    if (!m_main) {
        m_main = new MainWindow(*m_db, *m_session);
        connect(m_main, &MainWindow::vaultLocked, this, &App::onVaultLocked);
    }

    m_main->show();
    m_main->raise();
    m_main->activateWindow();

    if (m_login) {
        m_login->hide();
    }
}

void App::onUnlockRequested(const QString& password) {
    QByteArray key;
    if (!m_vault->unlockVault(password, key)) {
        handleAuthFailure();
        return;
    }

    AppSettings::resetFailedUnlockAttempts();
    m_session->setKey(std::move(key));
    if (m_login) {
        m_login->clearPassword();
    }
    showMain();
    m_main->onVaultUnlocked();
}

void App::onCreateVaultRequested(const QString& password) {
    if (m_vault->vaultExists()) {
        if (!DialogUtils::question(
                m_login,
                QStringLiteral("Create Vault"),
                QStringLiteral("A vault already exists. Creating a new vault will erase all existing notes.\n\nContinue?"))) {
            return;
        }

        m_db->close();
        const QString vaultPath = m_db->path();
        if (QFile::exists(vaultPath) && !QFile::remove(vaultPath)) {
            m_login->showError(QStringLiteral("Could not remove the existing vault file."));
            if (!m_db->open(vaultPath)) {
                m_login->showError(QStringLiteral("Could not reopen vault file."));
            }
            return;
        }
        if (!m_db->open(vaultPath)) {
            m_login->showError(QStringLiteral("Could not reset vault file."));
            return;
        }
        m_vault = std::make_unique<VaultRepository>(*m_db);
    }

    if (!m_vault->createVault(password)) {
        m_login->showError(QStringLiteral("Could not create vault. Try a different password."));
        return;
    }

    QByteArray key;
    if (!m_vault->unlockVault(password, key)) {
        m_login->showError(QStringLiteral("Vault created but unlock failed."));
        return;
    }

    m_session->setKey(std::move(key));
    if (m_login) {
        m_login->clearPassword();
    }
    showMain();
    m_main->onVaultUnlocked();
}

void App::onVaultLocked() {
    showLogin();
}

void App::handleAuthFailure() {
    if (!AppSettings::selfDestructEnabled()) {
        if (m_login) {
            m_login->showError(QStringLiteral("Incorrect master password."));
            m_login->clearPassword();
        }
        return;
    }

    AppSettings::incrementFailedUnlockAttempts();
    const int remaining = AppSettings::kMaxFailedUnlockAttempts
        - AppSettings::failedUnlockAttempts();

    if (remaining > 0) {
        if (m_login) {
            m_login->showError(
                QStringLiteral("Incorrect master password. %1 attempt(s) remaining.")
                    .arg(remaining));
            m_login->clearPassword();
        }
        return;
    }

    destroyVaultAfterFailedAttempts();
}

void App::destroyVaultAfterFailedAttempts() {
    AppSettings::resetFailedUnlockAttempts();

    if (m_main) {
        m_main->stopBridge();
        m_main->hide();
    }

    m_session->lock();

    m_db->close();
    const QString vaultPath = m_db->path();
    if (QFile::exists(vaultPath) && !QFile::remove(vaultPath)) {
        DialogUtils::critical(
            m_login,
            QStringLiteral("Vault Deletion Failed"),
            QStringLiteral(
                "Three failed unlock attempts were detected, but the vault file could not be removed. "
                "Your data may still be on disk."));
        if (!m_db->open(vaultPath)) {
            DialogUtils::critical(
                m_login,
                QStringLiteral("Vault Deletion Failed"),
                QStringLiteral("The vault database could not be reopened."));
        } else {
            m_vault = std::make_unique<VaultRepository>(*m_db);
        }
        if (m_login) {
            m_login->setVaultExists(m_vault && m_vault->vaultExists());
            m_login->clearPassword();
            m_login->show();
            m_login->raise();
            m_login->activateWindow();
        }
        return;
    }

    if (!m_db->open(vaultPath)) {
        DialogUtils::critical(
            m_login,
            QStringLiteral("Vault Destroyed"),
            QStringLiteral("The vault file was removed, but a new database could not be created."));
        if (m_login) {
            m_login->setVaultExists(false);
            m_login->clearPassword();
            m_login->show();
            m_login->raise();
            m_login->activateWindow();
        }
        return;
    }

    m_vault = std::make_unique<VaultRepository>(*m_db);

    DialogUtils::critical(
        m_login,
        QStringLiteral("Vault Destroyed"),
        QStringLiteral(
            "Three failed unlock attempts. The vault file was deleted from disk. "
            "This does not guarantee physical erasure."));

    if (m_login) {
        m_login->setVaultExists(false);
        m_login->clearPassword();
        m_login->show();
        m_login->raise();
        m_login->activateWindow();
    }
}
