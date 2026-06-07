#pragma once

#include "utils/VaultHealth.h"

#include <QWidget>

class QComboBox;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class QPushButton;
class QLabel;

class SettingsWindow : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    QString accentColor() const;
    void setAccentColor(const QString& hex);
    bool lineNumbers() const;
    bool wordWrap() const;
    bool autoLockEnabled() const;
    int autoLockMinutes() const;
    bool selfDestructEnabled() const;
    void setSelfDestructEnabled(bool enabled);
    bool browserBridgeEnabled() const;
    void setBrowserBridgeEnabled(bool enabled);
    void setVaultHealthReport(const VaultHealthReport& report);

signals:
    void accentChanged(const QString& hex);
    void lineNumbersChanged(bool enabled);
    void wordWrapChanged(bool enabled);
    void autoLockChanged(bool enabled, int minutes);
    void browserBridgeChanged(bool enabled);
    void hibpCheckChanged(bool enabled);
    void enableHelloUnlockRequested();
    void disableHelloUnlockRequested();
    void changePasswordRequested(const QString& current, const QString& newPass);
    void backupVaultRequested(const QString& path);
    void restoreVaultRequested(const QString& path);
    void importMarkdownRequested();
    void importCredentialsRequested();
    void exportNoteRequested();
    void exportAllMarkdownRequested();
    void exportEncryptedArchiveRequested(const QString& path);
    void refreshHealthRequested();
    void scanSecretsRequested();
    void redactExportRequested();
    void startRunbookRequested();
    void showKnowledgeGraphRequested();
    void showChronicleRequested();
    void exportGrimShareRequested();
    void importGrimShareRequested();
    void lockWorkChamberRequested();
    void unlockWorkChamberRequested();
    void webClipperChanged(bool enabled);
    void semanticSearchChanged(bool enabled);
    void backRequested();

private:
    void buildUi();
    void updateHealthLabels();

    QComboBox* m_accentCombo = nullptr;
    QCheckBox* m_lineNumbersCheck = nullptr;
    QCheckBox* m_wordWrapCheck = nullptr;
    QCheckBox* m_autoLockCheck = nullptr;
    QSpinBox* m_autoLockSpin = nullptr;
    QLineEdit* m_currentPassEdit = nullptr;
    QLineEdit* m_newPassEdit = nullptr;
    QCheckBox* m_selfDestructCheck = nullptr;
    QCheckBox* m_browserBridgeCheck = nullptr;
    QCheckBox* m_hibpCheck = nullptr;
    QCheckBox* m_clipperCheck = nullptr;
    QCheckBox* m_semanticSearchCheck = nullptr;
    QPushButton* m_enableHelloBtn = nullptr;
    QPushButton* m_disableHelloBtn = nullptr;
    QLabel* m_vaultSizeLabel = nullptr;
    QLabel* m_countsLabel = nullptr;
    QLabel* m_integrityLabel = nullptr;
    QLabel* m_bridgeHealthLabel = nullptr;
    QLabel* m_backupAgeLabel = nullptr;
    VaultHealthReport m_health;
};
