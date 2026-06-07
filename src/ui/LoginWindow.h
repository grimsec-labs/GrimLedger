#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;
class QProgressBar;
class QStackedWidget;
class CustomTitleBar;

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);

signals:
    void unlockRequested(const QString& password);
    void createVaultRequested(const QString& password);

private slots:
    void onUnlockClicked();
    void onCreateClicked();
    void onPasswordChanged(const QString& text);
    void onToggleMode();

public slots:
    void setVaultExists(bool exists);
    void showError(const QString& message);
    void clearPassword();

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    void buildUi();

    CustomTitleBar* m_titleBar = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QLineEdit* m_confirmEdit = nullptr;
    QPushButton* m_actionButton = nullptr;
    QPushButton* m_modeButton = nullptr;
    QLabel* m_errorLabel = nullptr;
    QLabel* m_strengthLabel = nullptr;
    QProgressBar* m_strengthBar = nullptr;
    QLabel* m_cursorLabel = nullptr;
    bool m_createMode = false;
    bool m_vaultExists = false;
};
