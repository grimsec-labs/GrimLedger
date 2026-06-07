#pragma once

#include <QWidget>
#include <QVector>
#include "models/Folder.h"
#include "models/Tag.h"

class QListWidget;

enum class SidebarSection {
    AllNotes,
    Favorites,
    Recent,
    Folder,
    Tag,
    Settings,
    Lock
};

class Sidebar : public QWidget {
    Q_OBJECT

public:
    explicit Sidebar(QWidget* parent = nullptr);

    void setFolders(const QVector<Folder>& folders);
    void setTags(const QVector<Tag>& tags);

signals:
    void sectionSelected(SidebarSection section, qint64 id = 0);
    void lockRequested();
    void newFolderRequested();

private slots:
    void onNavItemClicked(int row);

private:
    void buildUi();
    void rebuildFolderList();
    void rebuildTagList();

    QListWidget* m_navList = nullptr;
    QListWidget* m_folderList = nullptr;
    QListWidget* m_tagList = nullptr;
    QVector<Folder> m_folders;
    QVector<Tag> m_tags;
};
