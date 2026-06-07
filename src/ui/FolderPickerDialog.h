#pragma once

#include "ui/GrimDialog.h"
#include <QVector>
#include "models/Folder.h"

class QListWidget;
class QCheckBox;

class FolderPickerDialog : public GrimDialog {
    Q_OBJECT

public:
    explicit FolderPickerDialog(
        const QVector<Folder>& folders,
        qint64 defaultFolderId,
        qint64 currentFolderId,
        QWidget* parent = nullptr);

    qint64 selectedFolderId() const;
    bool setAsDefaultRequested() const;

private:
    void buildUi(const QVector<Folder>& folders, qint64 defaultFolderId, qint64 currentFolderId);

    QListWidget* m_folderList = nullptr;
    QCheckBox* m_defaultCheck = nullptr;
    qint64 m_selectedId = 0;
};
