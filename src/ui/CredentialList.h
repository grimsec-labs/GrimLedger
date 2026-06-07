#pragma once

#include <QWidget>
#include <QVector>
#include "models/CredentialSummary.h"

class QLineEdit;
class QListWidget;
class QPushButton;

class CredentialList : public QWidget {
    Q_OBJECT

public:
    explicit CredentialList(QWidget* parent = nullptr);

    void setCredentials(const QVector<CredentialSummary>& creds);
    void selectCredential(qint64 id);
    void clearSelection();
    qint64 currentCredentialId() const;

signals:
    void credentialSelected(qint64 id);
    void newCredentialRequested();
    void deleteCredentialRequested(qint64 id);
    void searchChanged(const QString& query);

private slots:
    void onItemClicked();

private:
    void buildUi();

    QLineEdit* m_searchEdit = nullptr;
    QListWidget* m_list = nullptr;
    QPushButton* m_newButton = nullptr;
    qint64 m_currentId = 0;
};
