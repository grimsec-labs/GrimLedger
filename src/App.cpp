#include "App.h"
#include "ui/LoginWindow.h"
#include "ui/MainWindow.h"
#include "storage/Database.h"
#include "storage/VaultRepository.h"
#include "security/VaultSession.h"
#include "security/CryptoManager.h"

#include <QMessageBox>
#include <QFile>

App::App(QObject* parent)
    : QObject(parent)
    , m_db(std::make_unique<Database>())
    , m_session(std::make_unique<VaultSession>()) {}

App::~App() = default;

void App::start() {
    if (!openDatabase()) {
        QMessageBox::critical(
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

    m_session->setKey(std::move(key));
    showMain();
}

void App::onCreateVaultRequested(const QString& password) {
    if (m_vault->vaultExists()) {
        const auto reply = QMessageBox::warning(
            m_login,
            QStringLiteral("Create Vault"),
            QStringLiteral("A vault already exists. Creating a new vault will erase all existing notes.\n\nContinue?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }

        m_db->close();
        QFile::remove(m_db->path());
        if (!m_db->open(m_db->path())) {
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
    showMain();
}

void App::onVaultLocked() {
    showLogin();
}

void App::handleAuthFailure() {
    if (m_login) {
        m_login->showError(QStringLiteral("Incorrect master password."));
        m_login->clearPassword();
    }
}
