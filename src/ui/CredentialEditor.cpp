#include "ui/CredentialEditor.h"
#include "utils/TimeUtils.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>

CredentialEditor::CredentialEditor(QWidget* parent)
    : QWidget(parent) {
    buildUi();
}

void CredentialEditor::buildUi() {
    setObjectName(QStringLiteral("CredentialEditor"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto* heading = new QLabel(QStringLiteral("⛨ VAULT KEY"), this);
    heading->setObjectName(QStringLiteral("LoginTitle"));

    auto* hint = new QLabel(
        QStringLiteral("Credentials are encrypted in your vault. Clipboard copies auto-clear after 20 seconds."),
        this);
    hint->setObjectName(QStringLiteral("WarningLabel"));
    hint->setWordWrap(true);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    m_labelEdit = new QLineEdit(this);
    m_labelEdit->setObjectName(QStringLiteral("TitleField"));
    m_labelEdit->setPlaceholderText(QStringLiteral("Site or service name"));
    connect(m_labelEdit, &QLineEdit::textChanged, this, &CredentialEditor::contentChanged);

    m_userEdit = new QLineEdit(this);
    m_userEdit->setObjectName(QStringLiteral("PasswordField"));
    m_userEdit->setPlaceholderText(QStringLiteral("Username or email"));
    connect(m_userEdit, &QLineEdit::textChanged, this, &CredentialEditor::contentChanged);

    m_passEdit = new QLineEdit(this);
    m_passEdit->setObjectName(QStringLiteral("PasswordField"));
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText(QStringLiteral("Password"));
    connect(m_passEdit, &QLineEdit::textChanged, this, &CredentialEditor::contentChanged);

    auto* passRow = new QHBoxLayout();
    passRow->addWidget(m_passEdit, 1);
    auto* genBtn = new QPushButton(QStringLiteral("Generate"), this);
    genBtn->setObjectName(QStringLiteral("SecondaryButton"));
    connect(genBtn, &QPushButton::clicked, this, &CredentialEditor::generatePasswordRequested);
    auto* copyPassBtn = new QPushButton(QStringLiteral("Copy"), this);
    copyPassBtn->setObjectName(QStringLiteral("SecondaryButton"));
    connect(copyPassBtn, &QPushButton::clicked, this, &CredentialEditor::copyPasswordRequested);
    passRow->addWidget(genBtn);
    passRow->addWidget(copyPassBtn);

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setObjectName(QStringLiteral("PasswordField"));
    m_urlEdit->setPlaceholderText(QStringLiteral("https://example.com"));
    connect(m_urlEdit, &QLineEdit::textChanged, this, &CredentialEditor::contentChanged);

    m_notesEdit = new QPlainTextEdit(this);
    m_notesEdit->setObjectName(QStringLiteral("MarkdownEditor"));
    m_notesEdit->setPlaceholderText(QStringLiteral("Security questions, backup codes, notes..."));
    m_notesEdit->setMaximumHeight(120);
    connect(m_notesEdit, &QPlainTextEdit::textChanged, this, &CredentialEditor::contentChanged);

    form->addRow(QStringLiteral("Label"), m_labelEdit);
    form->addRow(QStringLiteral("Username"), m_userEdit);
    form->addRow(QStringLiteral("Password"), passRow);
    form->addRow(QStringLiteral("URL"), m_urlEdit);
    form->addRow(QStringLiteral("Notes"), m_notesEdit);

    auto* btnRow = new QHBoxLayout();
    m_saveButton = new QPushButton(QStringLiteral("Save Vault Key"), this);
    m_saveButton->setObjectName(QStringLiteral("PrimaryButton"));
    connect(m_saveButton, &QPushButton::clicked, this, &CredentialEditor::saveRequested);

    auto* copyUserBtn = new QPushButton(QStringLiteral("Copy Username"), this);
    copyUserBtn->setObjectName(QStringLiteral("SecondaryButton"));
    connect(copyUserBtn, &QPushButton::clicked, this, &CredentialEditor::copyUsernameRequested);

    auto* delBtn = new QPushButton(QStringLiteral("Delete"), this);
    delBtn->setObjectName(QStringLiteral("SecondaryButton"));
    connect(delBtn, &QPushButton::clicked, this, &CredentialEditor::deleteRequested);

    btnRow->addWidget(m_saveButton);
    btnRow->addWidget(copyUserBtn);
    btnRow->addWidget(delBtn);
    btnRow->addStretch();

    m_statusLabel = new QLabel(QStringLiteral("Unsaved"), this);
    m_statusLabel->setObjectName(QStringLiteral("SaveStatus"));

    root->addWidget(heading);
    root->addWidget(hint);
    root->addLayout(form);
    root->addLayout(btnRow);
    root->addWidget(m_statusLabel);
    root->addStretch();
}

QString CredentialEditor::label() const { return m_labelEdit->text().trimmed(); }
QString CredentialEditor::username() const { return m_userEdit->text(); }
QString CredentialEditor::password() const { return m_passEdit->text(); }
QString CredentialEditor::url() const { return m_urlEdit->text().trimmed(); }
QString CredentialEditor::notes() const { return m_notesEdit->toPlainText(); }

void CredentialEditor::setLabel(const QString& text) { m_labelEdit->setText(text); }
void CredentialEditor::setUsername(const QString& text) { m_userEdit->setText(text); }
void CredentialEditor::setPassword(const QString& text) { m_passEdit->setText(text); }
void CredentialEditor::setUrl(const QString& text) { m_urlEdit->setText(text); }
void CredentialEditor::setNotes(const QString& text) { m_notesEdit->setPlainText(text); }

void CredentialEditor::setSavedState(bool saved, const QDateTime& updatedAt) {
    if (saved && updatedAt.isValid()) {
        m_statusLabel->setText(
            QStringLiteral("Saved · %1").arg(TimeUtils::formatTimestamp(updatedAt)));
    } else if (saved) {
        m_statusLabel->setText(QStringLiteral("Saved"));
    } else {
        m_statusLabel->setText(QStringLiteral("Unsaved changes"));
    }
}

void CredentialEditor::clearFields() {
    m_labelEdit->clear();
    m_userEdit->clear();
    m_passEdit->clear();
    m_urlEdit->clear();
    m_notesEdit->clear();
    m_statusLabel->setText(QStringLiteral("Select or create a vault key"));
}
