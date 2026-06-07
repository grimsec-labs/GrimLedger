#include "ui/SettingsWindow.h"
#include "ui/GrimFileDialog.h"
#include "utils/AppSettings.h"
#include "security/WindowsHelloUnlock.h"
#include "security/BreachCheck.h"

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

namespace {

QString formatBytes(qint64 bytes) {
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(bytes / 1024);
    }
    return QStringLiteral("%1 MB").arg(bytes / (1024 * 1024));
}

QString formatBackupAge(const QDateTime& when) {
    if (!when.isValid()) {
        return QStringLiteral("Never");
    }
    return when.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
}

} // namespace

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

    auto* health = new QGroupBox(QStringLiteral("Vault Health"), this);
    auto* healthForm = new QFormLayout(health);
    m_vaultSizeLabel = new QLabel(QStringLiteral("—"), this);
    m_countsLabel = new QLabel(QStringLiteral("—"), this);
    m_integrityLabel = new QLabel(QStringLiteral("—"), this);
    m_bridgeHealthLabel = new QLabel(QStringLiteral("—"), this);
    m_backupAgeLabel = new QLabel(QStringLiteral("—"), this);
    healthForm->addRow(QStringLiteral("Vault size:"), m_vaultSizeLabel);
    healthForm->addRow(QStringLiteral("Contents:"), m_countsLabel);
    healthForm->addRow(QStringLiteral("Integrity:"), m_integrityLabel);
    healthForm->addRow(QStringLiteral("Bridge:"), m_bridgeHealthLabel);
    healthForm->addRow(QStringLiteral("Last encrypted backup:"), m_backupAgeLabel);
    auto* refreshHealthBtn = new QPushButton(QStringLiteral("Refresh Health"), this);
    refreshHealthBtn->setObjectName(QStringLiteral("SecondaryButton"));
    connect(refreshHealthBtn, &QPushButton::clicked, this, &SettingsWindow::refreshHealthRequested);
    healthForm->addRow(refreshHealthBtn);

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
    m_autoLockCheck->setChecked(AppSettings::autoLockEnabled());
    m_autoLockSpin = new QSpinBox(this);
    m_autoLockSpin->setRange(1, 120);
    m_autoLockSpin->setValue(AppSettings::autoLockMinutes());
    m_autoLockSpin->setSuffix(QStringLiteral(" min"));
    connect(m_autoLockCheck, &QCheckBox::toggled, this, [this](bool on) {
        AppSettings::setAutoLockEnabled(on);
        emit autoLockChanged(on, m_autoLockSpin->value());
    });
    connect(m_autoLockSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        AppSettings::setAutoLockMinutes(v);
        emit autoLockChanged(m_autoLockCheck->isChecked(), v);
    });
    secForm->addRow(m_autoLockCheck, m_autoLockSpin);

    m_browserBridgeCheck = new QCheckBox(
        QStringLiteral("Enable browser bridge (Chrome/Edge extension)"), this);
    m_browserBridgeCheck->setChecked(AppSettings::browserBridgeEnabled());
    connect(m_browserBridgeCheck, &QCheckBox::toggled, this, [this](bool on) {
        AppSettings::setBrowserBridgeEnabled(on);
        emit browserBridgeChanged(on);
    });
    secForm->addRow(m_browserBridgeCheck);

    m_hibpCheck = new QCheckBox(
        QStringLiteral("Opt in to Have I Been Pwned k-anonymity checks"), this);
    m_hibpCheck->setChecked(AppSettings::hibpCheckEnabled());
    connect(m_hibpCheck, &QCheckBox::toggled, this, [this](bool on) {
        BreachCheck::setEnabled(on);
        emit hibpCheckChanged(on);
    });
    secForm->addRow(m_hibpCheck);

    auto* hibpWarn = new QLabel(
        QStringLiteral("When enabled, only the first 5 characters of a password SHA-1 hash are sent over the network."),
        this);
    hibpWarn->setObjectName(QStringLiteral("WarningLabel"));
    hibpWarn->setWordWrap(true);
    secForm->addRow(hibpWarn);

    if (WindowsHelloUnlock::isPlatformSupported()) {
        auto* helloRow = new QHBoxLayout();
        m_enableHelloBtn = new QPushButton(QStringLiteral("Enable Windows Hello Unlock"), this);
        m_disableHelloBtn = new QPushButton(QStringLiteral("Disable"), this);
        m_disableHelloBtn->setObjectName(QStringLiteral("SecondaryButton"));
        m_disableHelloBtn->setEnabled(WindowsHelloUnlock::isConfigured());
        connect(m_enableHelloBtn, &QPushButton::clicked, this, &SettingsWindow::enableHelloUnlockRequested);
        connect(m_disableHelloBtn, &QPushButton::clicked, this, [this]() {
            emit disableHelloUnlockRequested();
            m_disableHelloBtn->setEnabled(false);
        });
        helloRow->addWidget(m_enableHelloBtn);
        helloRow->addWidget(m_disableHelloBtn);
        secForm->addRow(QStringLiteral("Convenience unlock:"), helloRow);
    }

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

    auto* importCredBtn = new QPushButton(QStringLiteral("Import Credentials (Bitwarden/KeePass)"), this);
    connect(importCredBtn, &QPushButton::clicked, this, &SettingsWindow::importCredentialsRequested);

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
    dataLayout->addWidget(importCredBtn);
    dataLayout->addWidget(exportNoteBtn);
    dataLayout->addWidget(exportAllBtn);
    dataLayout->addWidget(exportEncBtn);

    layout->addWidget(backBtn, 0, Qt::AlignLeft);
    layout->addWidget(title);
    layout->addWidget(health);
    layout->addWidget(appearance);
    auto* workspace = new QGroupBox(QStringLiteral("Workspace Tools (Stage B)"), this);
    auto* workspaceLayout = new QVBoxLayout(workspace);

    m_clipperCheck = new QCheckBox(QStringLiteral("Enable encrypted web clipper (separate from fill)"), this);
    m_clipperCheck->setChecked(AppSettings::webClipperEnabled());
    connect(m_clipperCheck, &QCheckBox::toggled, this, [this](bool on) {
        AppSettings::setWebClipperEnabled(on);
        emit webClipperChanged(on);
    });
    workspaceLayout->addWidget(m_clipperCheck);

    m_semanticSearchCheck = new QCheckBox(QStringLiteral("Enable local semantic search (synonym expansion)"), this);
    m_semanticSearchCheck->setChecked(AppSettings::semanticSearchEnabled());
    connect(m_semanticSearchCheck, &QCheckBox::toggled, this, [this](bool on) {
        AppSettings::setSemanticSearchEnabled(on);
        emit semanticSearchChanged(on);
    });
    workspaceLayout->addWidget(m_semanticSearchCheck);

    auto* scanBtn = new QPushButton(QStringLiteral("Scan Notes for Secrets"), this);
    connect(scanBtn, &QPushButton::clicked, this, &SettingsWindow::scanSecretsRequested);
    auto* redactBtn = new QPushButton(QStringLiteral("Redacted Export (Current Note)"), this);
    connect(redactBtn, &QPushButton::clicked, this, &SettingsWindow::redactExportRequested);
    auto* runbookBtn = new QPushButton(QStringLiteral("Start Runbook Session"), this);
    connect(runbookBtn, &QPushButton::clicked, this, &SettingsWindow::startRunbookRequested);
    auto* graphBtn = new QPushButton(QStringLiteral("Knowledge Graph"), this);
    connect(graphBtn, &QPushButton::clicked, this, &SettingsWindow::showKnowledgeGraphRequested);
    auto* chronicleBtn = new QPushButton(QStringLiteral("Grim Chronicle"), this);
    connect(chronicleBtn, &QPushButton::clicked, this, &SettingsWindow::showChronicleRequested);
    auto* shareExportBtn = new QPushButton(QStringLiteral("Export GrimShare Package"), this);
    connect(shareExportBtn, &QPushButton::clicked, this, &SettingsWindow::exportGrimShareRequested);
    auto* shareImportBtn = new QPushButton(QStringLiteral("Import GrimShare Package"), this);
    connect(shareImportBtn, &QPushButton::clicked, this, &SettingsWindow::importGrimShareRequested);
    auto* lockWorkBtn = new QPushButton(QStringLiteral("Lock Work Chamber"), this);
    connect(lockWorkBtn, &QPushButton::clicked, this, &SettingsWindow::lockWorkChamberRequested);
    auto* unlockWorkBtn = new QPushButton(QStringLiteral("Unlock Work Chamber"), this);
    connect(unlockWorkBtn, &QPushButton::clicked, this, &SettingsWindow::unlockWorkChamberRequested);

    workspaceLayout->addWidget(scanBtn);
    workspaceLayout->addWidget(redactBtn);
    workspaceLayout->addWidget(runbookBtn);
    workspaceLayout->addWidget(graphBtn);
    workspaceLayout->addWidget(chronicleBtn);
    workspaceLayout->addWidget(shareExportBtn);
    workspaceLayout->addWidget(shareImportBtn);
    workspaceLayout->addWidget(lockWorkBtn);
    workspaceLayout->addWidget(unlockWorkBtn);

    layout->addWidget(security);
    layout->addWidget(data);
    layout->addWidget(workspace);
    layout->addStretch();
}

void SettingsWindow::setVaultHealthReport(const VaultHealthReport& report) {
    m_health = report;
    updateHealthLabels();
}

void SettingsWindow::updateHealthLabels() {
    m_vaultSizeLabel->setText(
        QStringLiteral("%1 on disk · %2 attachments (decoded est.)")
            .arg(formatBytes(m_health.vaultFileBytes))
            .arg(formatBytes(m_health.attachmentBytes)));
    m_countsLabel->setText(
        QStringLiteral("%1 notes · %2 vault keys")
            .arg(m_health.noteCount)
            .arg(m_health.credentialCount));
    const int totalIntegrity = m_health.credentialIntegrityErrors + m_health.noteIntegrityErrors;
    if (totalIntegrity == 0) {
        m_integrityLabel->setText(QStringLiteral("No integrity errors detected"));
    } else {
        m_integrityLabel->setText(
            QStringLiteral("%1 credential · %2 note integrity errors")
                .arg(m_health.credentialIntegrityErrors)
                .arg(m_health.noteIntegrityErrors));
    }
    const QString bridgeState = m_health.bridgeEnabled
        ? (m_health.bridgeListening ? QStringLiteral("Enabled and listening")
                                    : QStringLiteral("Enabled (not listening)"))
        : QStringLiteral("Disabled");
    m_bridgeHealthLabel->setText(bridgeState);
    m_backupAgeLabel->setText(
        QStringLiteral("Encrypted: %1 · Local .bak: %2")
            .arg(formatBackupAge(m_health.lastEncryptedBackupTime))
            .arg(formatBackupAge(m_health.lastLocalBackupTime)));
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

bool SettingsWindow::browserBridgeEnabled() const {
    return m_browserBridgeCheck->isChecked();
}

void SettingsWindow::setBrowserBridgeEnabled(bool enabled) {
    QSignalBlocker blocker(m_browserBridgeCheck);
    m_browserBridgeCheck->setChecked(enabled);
}
