#pragma once

#include <QWidget>
#include <QDateTime>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QCheckBox;

class CredentialEditor : public QWidget {
    Q_OBJECT

public:
    explicit CredentialEditor(QWidget* parent = nullptr);

    QString label() const;
    QString username() const;
    QString password() const;
    QString url() const;
    QString notes() const;
    bool allowSubdomains() const;
    bool integrityError() const;

    void setLabel(const QString& text);
    void setUsername(const QString& text);
    void setPassword(const QString& text);
    void setUrl(const QString& text);
    void setNotes(const QString& text);
    void setAllowSubdomains(bool enabled);
    void setIntegrityError(bool errored);
    void setSavedState(bool saved, const QDateTime& updatedAt = QDateTime());
    void clearFields();

signals:
    void contentChanged();
    void saveRequested();
    void deleteRequested();
    void generatePasswordRequested();
    void copyPasswordRequested();
    void copyUsernameRequested();

private:
    void buildUi();

    QLineEdit* m_labelEdit = nullptr;
    QLineEdit* m_userEdit = nullptr;
    QLineEdit* m_passEdit = nullptr;
    QLineEdit* m_urlEdit = nullptr;
    QPlainTextEdit* m_notesEdit = nullptr;
    QPushButton* m_saveButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QCheckBox* m_subdomainCheck = nullptr;
    bool m_integrityError = false;
};
