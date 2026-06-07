#include "ui/CredentialList.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QListWidgetItem>

CredentialList::CredentialList(QWidget* parent)
    : QWidget(parent) {
    buildUi();
}

void CredentialList::buildUi() {
    setObjectName(QStringLiteral("CredentialListPanel"));
    setMinimumWidth(260);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName(QStringLiteral("SearchField"));
    m_searchEdit->setPlaceholderText(QStringLiteral("Search vault keys..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CredentialList::searchChanged);

    m_newButton = new QPushButton(QStringLiteral("+ New Vault Key"), this);
    m_newButton->setObjectName(QStringLiteral("PrimaryButton"));
    connect(m_newButton, &QPushButton::clicked, this, &CredentialList::newCredentialRequested);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("CredentialListWidget"));
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem*) {
        onItemClicked();
    });
    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) {
            return;
        }
        onItemClicked();
    });

    layout->addWidget(m_searchEdit);
    layout->addWidget(m_newButton);
    layout->addWidget(m_list, 1);
}

void CredentialList::setCredentials(const QVector<CredentialSummary>& creds) {
    m_list->clear();
    for (const CredentialSummary& c : creds) {
        QString label = c.label.isEmpty() ? QStringLiteral("(Unnamed)") : c.label;
        if (!c.username.isEmpty()) {
            label += QStringLiteral(" — ") + c.username;
        }
        auto* item = new QListWidgetItem(QStringLiteral("⛨ ") + label);
        item->setData(Qt::UserRole, c.id);
        if (!c.url.isEmpty()) {
            item->setToolTip(c.url);
        }
        m_list->addItem(item);
    }
}

void CredentialList::clearSelection() {
    m_list->clearSelection();
    m_currentId = 0;
}

void CredentialList::selectCredential(qint64 id) {
    for (int i = 0; i < m_list->count(); ++i) {
        auto* item = m_list->item(i);
        if (item && item->data(Qt::UserRole).toLongLong() == id) {
            m_list->setCurrentRow(i);
            m_currentId = id;
            return;
        }
    }
}

qint64 CredentialList::currentCredentialId() const {
    return m_currentId;
}

void CredentialList::onItemClicked() {
    auto* item = m_list->currentItem();
    if (!item) {
        return;
    }
    m_currentId = item->data(Qt::UserRole).toLongLong();
    emit credentialSelected(m_currentId);
}
