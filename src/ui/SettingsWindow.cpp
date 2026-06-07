#include "ui/SettingsWindow.h"
#include "ui/GrimFileDialog.h"
#include "utils/AppSettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QSignalBlocker>

SettingsWindow::SettingsWindow(QWidget* parent)
    : QWidget(parent) {
    buildUi();
}

void SettingsWindow::buildUi() {
    setObjectName(QStringLiteral("SettingsWindow"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("⚙ Vault Settings"), this);
    title->setObjectName(QStringLiteral("SettingsTitle"));

    auto* backBtn = new QPushButton(QStringLiteral("← Back"), this);
    backBtn->setObjectName(QStringLiteral("SecondaryButton"));
    connect(backBtn, &QPushButton::clicked, this, &SettingsWindow::backRequested);

    auto* appearance = new QGroupBox(QStringLiteral("Appearance"), this);
    auto* appForm = new QFormLayout(appearance);

    m_accentCombo = new QComboBox(this);
    m_accentCombo->addItem(QStringLiteral("Hellfire Red"), QStringLiteral("#cc2200"));
    m_accentCombo->addItem(QStringLiteral("Ember Orange"), QStringLiteral("#ff6600"));
    m_accentCombo->addItem(QStringLiteral("Terminal Green"), QStringLiteral("#00cc66"));
    m_accentCombo->addItem(QStringLiteral("Abyss Purple"), QStringLiteral("#8844cc"));
    connect(m_accentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        emit accentChanged(m_accentCombo->currentData().toString());
    });
    appForm->addRow(QStringLiteral("Accent color:"), m_accentCombo);

    m_lineNumbersCheck = new QCheckBox(QStringLiteral("Show line numbers"), this);
    connect(m_lineNumbersCheck, &QCheckBox::toggled, this, &SettingsWindow::lineNumbersChanged);
    appForm->addRow(m_lineNumbersCheck);

    m_wordWrapCheck = new QCheckBox(QStringLiteral("Word wrap in editor"), this);
    m_wordWrapCheck->setChecked(true);
    connect(m_wordWrapCheck, &QCheckBox::toggled, this, &SettingsWindow::wordWrapChanged);
    appForm->addRow(m_wordWrapCheck);

    auto* security = new QGroupBox(QStringLiteral("Security"), this);
    auto* secForm = new QFormLayout(security);

    m_autoLockCheck = new QCheckBox(QStringLiteral("Auto-lock after inactivity"), this);
    m_autoLockCheck->setChecked(true);
    m_autoLockSpin = new QSpinBox(this);
    m_autoLockSpin->setRange(1, 120);
    m_autoLockSpin->setValue(15);
    m_autoLockSpin->setSuffix(QStringLiteral(" min"));
    connect(m_autoLockCheck, &QCheckBox::toggled, this, [this](bool on) {
        emit autoLockChanged(on, m_autoLockSpin->value());
    });
    connect(m_autoLockSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        emit autoLockChanged(m_autoLockCheck->isChecked(), v);
    });
    secForm->addRow(m_autoLockCheck, m_autoLockSpin);

    m_selfDestructCheck = new QCheckBox(
        QStringLiteral("Destroy vault after 3 failed unlock attempts"), this);
    m_selfDestructCheck->setChecked(AppSettings::selfDestructEnabled());
    connect(m_selfDestructCheck, &QCheckBox::toggled, this, [this](bool on) {
        AppSettings::setSelfDestructEnabled(on);
    });
    secForm->addRow(m_selfDestructCheck);

    auto* selfDestructWarn = new QLabel(
        QStringLiteral("⚠ All notes will be permanently lost. Back up your vault first."), this);
    selfDestructWarn->setObjectName(QStringLiteral("WarningLabel"));
    selfDestructWarn->setWordWrap(true);
    secForm->addRow(selfDestructWarn);

    m_currentPassEdit = new QLineEdit(this);
    m_currentPassEdit->setEchoMode(QLineEdit::Password);
    m_currentPassEdit->setPlaceholderText(QStringLiteral("Current master password"));
    m_newPassEdit = new QLineEdit(this);
    m_newPassEdit->setEchoMode(QLineEdit::Password);
    m_newPassEdit->setPlaceholderText(QStringLiteral("New master password"));

    auto* changePassBtn = new QPushButton(QStringLiteral("Change Master Password"), this);
    changePassBtn->setObjectName(QStringLiteral("PrimaryButton"));
    connect(changePassBtn, &QPushButton::clicked, this, [this]() {
        emit changePasswordRequested(m_currentPassEdit->text(), m_newPassEdit->text());
        m_currentPassEdit->clear();
        m_newPassEdit->clear();
    });
    secForm->addRow(QStringLiteral("Current:"), m_currentPassEdit);
    secForm->addRow(QStringLiteral("New:"), m_newPassEdit);
    secForm->addRow(changePassBtn);

    auto* warn = new QLabel(
        QStringLiteral("⚠ Lost master passwords cannot be recovered."), this);
    warn->setObjectName(QStringLiteral("WarningLabel"));
    secForm->addRow(warn);

    auto* data = new QGroupBox(QStringLiteral("Data"), this);
    auto* dataLayout = new QVBoxLayout(data);

    auto* backupBtn = new QPushButton(QStringLiteral("Backup Encrypted Vault"), this);
    connect(backupBtn, &QPushButton::clicked, this, [this]() {
        const QString path = GrimFileDialog::getSaveFileName(
            this, QStringLiteral("Backup Vault"),
            QStringLiteral("grimledger-backup.grimbak"),
            QStringLiteral("GrimLedger Backup (*.grimbak)"));
        if (!path.isEmpty()) emit backupVaultRequested(path);
    });

    auto* restoreBtn = new QPushButton(QStringLiteral("Restore Encrypted Backup"), this);
    connect(restoreBtn, &QPushButton::clicked, this, [this]() {
        const QString path = GrimFileDialog::getOpenFileName(
            this, QStringLiteral("Restore Backup"),
            QString(),
            QStringLiteral("GrimLedger Backup (*.grimbak)"));
        if (!path.isEmpty()) emit restoreVaultRequested(path);
    });

    auto* importBtn = new QPushButton(QStringLiteral("Import Markdown Files"), this);
    connect(importBtn, &QPushButton::clicked, this, &SettingsWindow::importMarkdownRequested);

    auto* exportNoteBtn = new QPushButton(QStringLiteral("Export Selected Note"), this);
    connect(exportNoteBtn, &QPushButton::clicked, this, &SettingsWindow::exportNoteRequested);

    auto* exportAllBtn = new QPushButton(QStringLiteral("Export All Notes (.md)"), this);
    connect(exportAllBtn, &QPushButton::clicked, this, &SettingsWindow::exportAllMarkdownRequested);

    auto* exportEncBtn = new QPushButton(QStringLiteral("Export Encrypted Archive"), this);
    connect(exportEncBtn, &QPushButton::clicked, this, [this]() {
        const QString path = GrimFileDialog::getSaveFileName(
            this, QStringLiteral("Export Archive"),
            QStringLiteral("grimledger-archive.grimbak"),
            QStringLiteral("GrimLedger Archive (*.grimbak)"));
        if (!path.isEmpty()) emit exportEncryptedArchiveRequested(path);
    });

    dataLayout->addWidget(backupBtn);
    dataLayout->addWidget(restoreBtn);
    dataLayout->addWidget(importBtn);
    dataLayout->addWidget(exportNoteBtn);
    dataLayout->addWidget(exportAllBtn);
    dataLayout->addWidget(exportEncBtn);

    layout->addWidget(backBtn, 0, Qt::AlignLeft);
    layout->addWidget(title);
    layout->addWidget(appearance);
    layout->addWidget(security);
    layout->addWidget(data);
    layout->addStretch();
}

QString SettingsWindow::accentColor() const {
    return m_accentCombo->currentData().toString();
}

void SettingsWindow::setAccentColor(const QString& hex) {
    QSignalBlocker blocker(m_accentCombo);
    for (int i = 0; i < m_accentCombo->count(); ++i) {
        if (m_accentCombo->itemData(i).toString().compare(hex, Qt::CaseInsensitive) == 0) {
            m_accentCombo->setCurrentIndex(i);
            return;
        }
    }
}

bool SettingsWindow::lineNumbers() const { return m_lineNumbersCheck->isChecked(); }
bool SettingsWindow::wordWrap() const { return m_wordWrapCheck->isChecked(); }
bool SettingsWindow::autoLockEnabled() const { return m_autoLockCheck->isChecked(); }
int SettingsWindow::autoLockMinutes() const { return m_autoLockSpin->value(); }

bool SettingsWindow::selfDestructEnabled() const {
    return m_selfDestructCheck->isChecked();
}

void SettingsWindow::setSelfDestructEnabled(bool enabled) {
    QSignalBlocker blocker(m_selfDestructCheck);
    m_selfDestructCheck->setChecked(enabled);
}
