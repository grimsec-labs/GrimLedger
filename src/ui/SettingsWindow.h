#pragma once

#include <QWidget>

class QComboBox;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class QPushButton;

class SettingsWindow : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr);

    QString accentColor() const;
    bool lineNumbers() const;
    bool wordWrap() const;
    bool autoLockEnabled() const;
    int autoLockMinutes() const;

signals:
    void accentChanged(const QString& hex);
    void lineNumbersChanged(bool enabled);
    void wordWrapChanged(bool enabled);
    void autoLockChanged(bool enabled, int minutes);
    void changePasswordRequested(const QString& current, const QString& newPass);
    void backupVaultRequested(const QString& path);
    void restoreVaultRequested(const QString& path);
    void importMarkdownRequested();
    void exportNoteRequested();
    void exportAllMarkdownRequested();
    void exportEncryptedArchiveRequested(const QString& path);
    void backRequested();

private:
    void buildUi();

    QComboBox* m_accentCombo = nullptr;
    QCheckBox* m_lineNumbersCheck = nullptr;
    QCheckBox* m_wordWrapCheck = nullptr;
    QCheckBox* m_autoLockCheck = nullptr;
    QSpinBox* m_autoLockSpin = nullptr;
    QLineEdit* m_currentPassEdit = nullptr;
    QLineEdit* m_newPassEdit = nullptr;
};
