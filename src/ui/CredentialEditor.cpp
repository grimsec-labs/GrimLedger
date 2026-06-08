#include "ui/CredentialEditor.h"
#include "utils/TimeUtils.h"
#include "utils/TotpGenerator.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QScrollArea>
#include <QFrame>
#include <QSizePolicy>
#include <QTimer>

CredentialEditor::CredentialEditor(QWidget* parent)
    : QWidget(parent) {
    buildUi();
}

void CredentialEditor::buildUi() {
    setObjectName(QStringLiteral("CredentialEditor"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("CredentialScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("CredentialScrollContent"));
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto* heading = new QLabel(QStringLiteral("⛨ VAULT KEY"), this);
    heading->setObjectName(QStringLiteral("LoginTitle"));

    auto* hint = new QLabel(
        QStringLiteral("Credentials are encrypted in your vault. Clipboard clear is best-effort and may not erase OS clipboard history."),
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
    auto* breachBtn = new QPushButton(QStringLiteral("Check Breach"), this);
    breachBtn->setObjectName(QStringLiteral("SecondaryButton"));
    connect(breachBtn, &QPushButton::clicked, this, &CredentialEditor::checkBreachRequested);
    passRow->addWidget(genBtn);
    passRow->addWidget(copyPassBtn);
    passRow->addWidget(breachBtn);

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setObjectName(QStringLiteral("PasswordField"));
    m_urlEdit->setPlaceholderText(QStringLiteral("https://example.com"));
    connect(m_urlEdit, &QLineEdit::textChanged, this, &CredentialEditor::contentChanged);

    m_trustCombo = new QComboBox(this);
    m_trustCombo->addItem(fillTrustLevelLabel(FillTrustLevel::ExactOrigin),
        static_cast<int>(FillTrustLevel::ExactOrigin));
    m_trustCombo->addItem(fillTrustLevelLabel(FillTrustLevel::AllowSubdomains),
        static_cast<int>(FillTrustLevel::AllowSubdomains));
    m_trustCombo->addItem(fillTrustLevelLabel(FillTrustLevel::UsernameOnly),
        static_cast<int>(FillTrustLevel::UsernameOnly));
    m_trustCombo->addItem(fillTrustLevelLabel(FillTrustLevel::ManualOnly),
        static_cast<int>(FillTrustLevel::ManualOnly));
    connect(m_trustCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &CredentialEditor::contentChanged);

    m_totpEdit = new QLineEdit(this);
    m_totpEdit->setObjectName(QStringLiteral("PasswordField"));
    m_totpEdit->setPlaceholderText(QStringLiteral("Base32 TOTP secret (optional)"));
    connect(m_totpEdit, &QLineEdit::textChanged, this, [this]() {
        refreshTotpCode();
        emit contentChanged();
    });

    m_totpCodeLabel = new QLabel(QStringLiteral("—"), this);
    m_totpCodeLabel->setObjectName(QStringLiteral("SaveStatus"));

    auto* totpRow = new QHBoxLayout();
    totpRow->addWidget(m_totpEdit, 1);
    auto* copyTotpBtn = new QPushButton(QStringLiteral("Copy Code"), this);
    copyTotpBtn->setObjectName(QStringLiteral("SecondaryButton"));
    connect(copyTotpBtn, &QPushButton::clicked, this, &CredentialEditor::copyTotpRequested);
    totpRow->addWidget(copyTotpBtn);
    totpRow->addWidget(m_totpCodeLabel);

    m_notesEdit = new QPlainTextEdit(this);
    m_notesEdit->setObjectName(QStringLiteral("MarkdownEditor"));
    m_notesEdit->setPlaceholderText(QStringLiteral("Security questions, backup codes, notes..."));
    m_notesEdit->setMaximumHeight(120);
    connect(m_notesEdit, &QPlainTextEdit::textChanged, this, &CredentialEditor::contentChanged);

    form->addRow(QStringLiteral("Label"), m_labelEdit);
    form->addRow(QStringLiteral("Username"), m_userEdit);
    form->addRow(QStringLiteral("Password"), passRow);
    form->addRow(QStringLiteral("URL"), m_urlEdit);
    form->addRow(QStringLiteral("Fill trust"), m_trustCombo);
    form->addRow(QStringLiteral("TOTP secret"), totpRow);
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

    m_totpTimer = new QTimer(this);
    connect(m_totpTimer, &QTimer::timeout, this, &CredentialEditor::refreshTotpCode);
    m_totpTimer->start(1000);

    root->addWidget(heading);
    root->addWidget(hint);
    root->addLayout(form);
    root->addLayout(btnRow);
    root->addWidget(m_statusLabel);
    root->addStretch();

    scroll->setWidget(content);
    outer->addWidget(scroll);
}

QString CredentialEditor::label() const { return m_labelEdit->text().trimmed(); }
QString CredentialEditor::username() const { return m_userEdit->text(); }
QString CredentialEditor::password() const { return m_passEdit->text(); }
QString CredentialEditor::url() const { return m_urlEdit->text().trimmed(); }
QString CredentialEditor::notes() const { return m_notesEdit->toPlainText(); }
QString CredentialEditor::totpSecret() const {
    return TotpGenerator::normalizeSecret(m_totpEdit->text());
}
FillTrustLevel CredentialEditor::fillTrustLevel() const {
    return fillTrustLevelFromInt(m_trustCombo->currentData().toInt());
}

void CredentialEditor::setLabel(const QString& text) { m_labelEdit->setText(text); }
void CredentialEditor::setUsername(const QString& text) { m_userEdit->setText(text); }
void CredentialEditor::setPassword(const QString& text) { m_passEdit->setText(text); }
void CredentialEditor::setUrl(const QString& text) { m_urlEdit->setText(text); }
void CredentialEditor::setNotes(const QString& text) { m_notesEdit->setPlainText(text); }
void CredentialEditor::setTotpSecret(const QString& text) {
    m_totpEdit->setText(text);
    refreshTotpCode();
}
void CredentialEditor::setFillTrustLevel(FillTrustLevel level) {
    for (int i = 0; i < m_trustCombo->count(); ++i) {
        if (m_trustCombo->itemData(i).toInt() == static_cast<int>(level)) {
            m_trustCombo->setCurrentIndex(i);
            return;
        }
    }
}

bool CredentialEditor::integrityError() const {
    return m_integrityError;
}

void CredentialEditor::setIntegrityError(bool errored) {
    m_integrityError = errored;
    const bool enabled = !errored;
    m_labelEdit->setEnabled(enabled);
    m_userEdit->setEnabled(enabled);
    m_passEdit->setEnabled(enabled);
    m_urlEdit->setEnabled(enabled);
    m_totpEdit->setEnabled(enabled);
    m_notesEdit->setEnabled(enabled);
    m_trustCombo->setEnabled(enabled);
    m_saveButton->setEnabled(enabled);
    if (errored) {
        m_statusLabel->setText(QStringLiteral("Integrity error — this vault key cannot be edited."));
    }
}

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
    m_integrityError = false;
    m_labelEdit->clear();
    m_userEdit->clear();
    m_passEdit->clear();
    m_urlEdit->clear();
    m_totpEdit->clear();
    m_notesEdit->clear();
    m_trustCombo->setCurrentIndex(0);
    setIntegrityError(false);
    refreshTotpCode();
    m_statusLabel->setText(QStringLiteral("Select or create a vault key"));
}

void CredentialEditor::refreshTotpCode() {
    const QString secret = totpSecret();
    if (!TotpGenerator::isValidSecret(secret)) {
        m_totpCodeLabel->setText(QStringLiteral("—"));
        return;
    }
    const QString code = TotpGenerator::currentCode(secret);
    const int remaining = TotpGenerator::secondsRemaining();
    m_totpCodeLabel->setText(
        QStringLiteral("%1 · %2s").arg(code, QString::number(remaining)));
}
