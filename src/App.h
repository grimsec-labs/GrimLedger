#pragma once

#include <QObject>
#include <memory>

class LoginWindow;
class MainWindow;
class Database;
class VaultSession;
class VaultRepository;

class App : public QObject {
    Q_OBJECT

public:
    explicit App(QObject* parent = nullptr);
    ~App() override;

    void start();

private slots:
    void onUnlockRequested(const QString& password);
    void onCreateVaultRequested(const QString& password);
    void onVaultLocked();

private:
    bool openDatabase();
    void showLogin();
    void showMain();
    void handleAuthFailure();
    void destroyVaultAfterFailedAttempts();

    std::unique_ptr<Database> m_db;
    std::unique_ptr<VaultSession> m_session;
    std::unique_ptr<VaultRepository> m_vault;

    LoginWindow* m_login = nullptr;
    MainWindow* m_main = nullptr;
};
