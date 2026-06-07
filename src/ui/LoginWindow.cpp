#include "ui/LoginWindow.h"
#include "ui/CustomTitleBar.h"
#include "ui/FramelessResize.h"
#include "security/PasswordManager.h"
#include "security/WindowsHelloUnlock.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>
#include <QProgressBar>
#include <QStackedWidget>
#include <QFrame>
#include <QTimer>

LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    buildUi();
}

bool LoginWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (FramelessResize::handleNativeEvent(this, eventType, message, result)) {
        return true;
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void LoginWindow::buildUi() {
    setObjectName(QStringLiteral("LoginWindow"));
    setMinimumSize(520, 560);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_titleBar = new CustomTitleBar(this);
    m_titleBar->setTitle(QStringLiteral("GRIMLEDGER"));
    m_titleBar->setSubtitle(QStringLiteral("// vault access"));
    m_titleBar->setMaximizeEnabled(false);

    auto* content = new QWidget(this);
    content->setObjectName(QStringLiteral("LoginContent"));

    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(40, 24, 40, 40);
    root->setSpacing(16);

    outer->addWidget(m_titleBar);
    outer->addWidget(content, 1);

    auto* logo = new QLabel(this);
    logo->setObjectName(QStringLiteral("LoginLogo"));
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(
        QPixmap(QStringLiteral(":/logo.png"))
            .scaled(400, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto* prompt = new QLabel(QStringLiteral("> enter master key:"), this);
    prompt->setObjectName(QStringLiteral("LoginPrompt"));

    m_cursorLabel = new QLabel(QStringLiteral("_"), this);
    m_cursorLabel->setObjectName(QStringLiteral("BlinkCursor"));
    auto* cursorTimer = new QTimer(this);
    connect(cursorTimer, &QTimer::timeout, this, [this]() {
        m_cursorLabel->setVisible(!m_cursorLabel->isVisible());
    });
    cursorTimer->start(530);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setObjectName(QStringLiteral("PasswordField"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(QStringLiteral("Master password"));
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &LoginWindow::onPasswordChanged);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::onUnlockClicked);

    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setObjectName(QStringLiteral("PasswordField"));
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setPlaceholderText(QStringLiteral("Confirm master password"));
    m_confirmEdit->hide();

    m_strengthBar = new QProgressBar(this);
    m_strengthBar->setObjectName(QStringLiteral("StrengthBar"));
    m_strengthBar->setRange(0, 100);
    m_strengthBar->setTextVisible(false);
    m_strengthBar->hide();

    m_strengthLabel = new QLabel(this);
    m_strengthLabel->setObjectName(QStringLiteral("StrengthLabel"));
    m_strengthLabel->hide();

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("ErrorLabel"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    m_actionButton = new QPushButton(QStringLiteral("Unlock Vault"), this);
    m_actionButton->setObjectName(QStringLiteral("PrimaryButton"));
    connect(m_actionButton, &QPushButton::clicked, this, &LoginWindow::onUnlockClicked);

    m_helloButton = new QPushButton(QStringLiteral("Unlock with Windows Hello"), this);
    m_helloButton->setObjectName(QStringLiteral("SecondaryButton"));
    m_helloButton->hide();
    connect(m_helloButton, &QPushButton::clicked, this, &LoginWindow::helloUnlockRequested);

    m_modeButton = new QPushButton(QStringLiteral("Create New Vault"), this);
    m_modeButton->setObjectName(QStringLiteral("SecondaryButton"));
    connect(m_modeButton, &QPushButton::clicked, this, &LoginWindow::onToggleMode);

    auto* warning = new QLabel(
        QStringLiteral("⚠ Lost master passwords cannot be recovered."), this);
    warning->setObjectName(QStringLiteral("WarningLabel"));
    warning->setWordWrap(true);
    warning->setAlignment(Qt::AlignCenter);

    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName(QStringLiteral("Divider"));

    root->addStretch();
    root->addWidget(logo);
    root->addSpacing(8);
    root->addWidget(line);
    root->addSpacing(12);
    root->addWidget(prompt);

    auto* passRow = new QHBoxLayout();
    passRow->addWidget(m_passwordEdit);
    passRow->addWidget(m_cursorLabel);
    root->addLayout(passRow);
    root->addWidget(m_confirmEdit);
    root->addWidget(m_strengthBar);
    root->addWidget(m_strengthLabel);
    root->addWidget(m_errorLabel);
    root->addWidget(m_actionButton);
    root->addWidget(m_helloButton);
    root->addWidget(m_modeButton);
    root->addWidget(warning);
    root->addStretch();
}

void LoginWindow::setVaultExists(bool exists) {
    m_vaultExists = exists;
    if (!exists) {
        m_createMode = true;
        m_confirmEdit->show();
        m_strengthBar->show();
        m_strengthLabel->show();
        m_actionButton->setText(QStringLiteral("Create Vault"));
        m_modeButton->hide();
    } else {
        m_createMode = false;
        m_confirmEdit->hide();
        m_strengthBar->hide();
        m_strengthLabel->hide();
        m_actionButton->setText(QStringLiteral("Unlock Vault"));
        m_modeButton->setText(QStringLiteral("Create New Vault"));
        m_modeButton->show();
        m_helloButton->setVisible(
            WindowsHelloUnlock::isPlatformSupported() && WindowsHelloUnlock::isConfigured());
    }
}

void LoginWindow::showError(const QString& message) {
    m_errorLabel->setText(message);
    m_errorLabel->show();
}

void LoginWindow::clearPassword() {
    m_passwordEdit->clear();
    m_confirmEdit->clear();
    m_errorLabel->hide();
}

void LoginWindow::onPasswordChanged(const QString& text) {
    if (!m_createMode) return;
    const auto result = PasswordManager::evaluate(text);
    m_strengthBar->setValue(result.score);
    m_strengthLabel->setText(result.label);
    m_strengthLabel->setStyleSheet(
        QStringLiteral("color: %1;").arg(result.color));
}

void LoginWindow::onToggleMode() {
    m_createMode = !m_createMode;
    m_errorLabel->hide();

    if (m_createMode) {
        m_confirmEdit->show();
        m_strengthBar->show();
        m_strengthLabel->show();
        m_actionButton->setText(QStringLiteral("Create Vault"));
        m_modeButton->setText(QStringLiteral("Back to Unlock"));
    } else {
        m_confirmEdit->hide();
        m_strengthBar->hide();
        m_strengthLabel->hide();
        m_actionButton->setText(QStringLiteral("Unlock Vault"));
        m_modeButton->setText(QStringLiteral("Create New Vault"));
    }
}

void LoginWindow::onUnlockClicked() {
    m_errorLabel->hide();
    const QString password = m_passwordEdit->text();

    if (m_createMode || !m_vaultExists) {
        onCreateClicked();
        return;
    }

    if (password.isEmpty()) {
        showError(QStringLiteral("Enter your master password."));
        return;
    }

    emit unlockRequested(password);
}

void LoginWindow::onCreateClicked() {
    const QString password = m_passwordEdit->text();
    const QString confirm = m_confirmEdit->text();

    QString err;
    if (!PasswordManager::isValidVaultPassword(password, &err)) {
        showError(err);
        return;
    }

    if (m_createMode && password != confirm) {
        showError(QStringLiteral("Passwords do not match."));
        return;
    }

    if (!m_vaultExists || m_createMode) {
        emit createVaultRequested(password);
    }
}
