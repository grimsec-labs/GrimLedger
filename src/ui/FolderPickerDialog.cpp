#include "ui/FolderPickerDialog.h"

#include <QLabel>
#include <QListWidget>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

FolderPickerDialog::FolderPickerDialog(
    const QVector<Folder>& folders,
    qint64 defaultFolderId,
    qint64 currentFolderId,
    QWidget* parent)
    : GrimDialog(QStringLiteral("Save to Folder"), parent) {
    setMinimumWidth(360);
    buildUi(folders, defaultFolderId, currentFolderId);
}

void FolderPickerDialog::buildUi(
    const QVector<Folder>& folders,
    qint64 defaultFolderId,
    qint64 currentFolderId) {
    auto* prompt = new QLabel(
        QStringLiteral("Choose a folder for this note:"), this);
    prompt->setObjectName(QStringLiteral("DialogPrompt"));

    m_folderList = new QListWidget(this);
    m_folderList->setObjectName(QStringLiteral("FolderPickerList"));

    int selectRow = 0;
    int row = 0;
    for (const Folder& folder : folders) {
        QString label = folder.name;
        if (folder.id == defaultFolderId) {
            label += QStringLiteral("  ★ default");
        }
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, folder.id);
        m_folderList->addItem(item);

        const qint64 pickId = currentFolderId > 0 ? currentFolderId : defaultFolderId;
        if (folder.id == pickId) {
            selectRow = row;
            m_selectedId = folder.id;
        }
        ++row;
    }

    if (m_folderList->count() > 0) {
        m_folderList->setCurrentRow(selectRow);
    }

    connect(m_folderList, &QListWidget::currentRowChanged, this, [this](int index) {
        auto* item = m_folderList->item(index);
        if (item) {
            m_selectedId = item->data(Qt::UserRole).toLongLong();
        }
    });

    m_defaultCheck = new QCheckBox(
        QStringLiteral("Set selected folder as default"), this);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("Save && Close"));
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("PrimaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &FolderPickerDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &FolderPickerDialog::reject);

    contentLayout()->addWidget(prompt);
    contentLayout()->addWidget(m_folderList, 1);
    contentLayout()->addWidget(m_defaultCheck);
    contentLayout()->addWidget(buttons);
}

qint64 FolderPickerDialog::selectedFolderId() const {
    return m_selectedId;
}

bool FolderPickerDialog::setAsDefaultRequested() const {
    return m_defaultCheck->isChecked();
}
