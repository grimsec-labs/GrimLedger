#include "ui/Sidebar.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent) {
    buildUi();
}

void Sidebar::buildUi() {
    setObjectName(QStringLiteral("Sidebar"));
    setFixedWidth(200);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 12, 8, 12);
    layout->setSpacing(8);

    auto* brand = new QLabel(QStringLiteral("◆ GRIM"), this);
    brand->setObjectName(QStringLiteral("SidebarBrand"));

    m_navList = new QListWidget(this);
    m_navList->setObjectName(QStringLiteral("NavList"));
    m_navList->addItem(QStringLiteral("All Notes"));
    m_navList->addItem(QStringLiteral("★ Favorites"));
    m_navList->addItem(QStringLiteral("◷ Recent"));
    m_navList->addItem(QStringLiteral("⚙ Settings"));
    m_navList->addItem(QStringLiteral("⛨ Lock Vault"));
    connect(m_navList, &QListWidget::currentRowChanged, this, &Sidebar::onNavItemClicked);

    auto* folderLabel = new QLabel(QStringLiteral("FOLDERS"), this);
    folderLabel->setObjectName(QStringLiteral("SectionLabel"));

    m_folderList = new QListWidget(this);
    m_folderList->setObjectName(QStringLiteral("FolderList"));

    auto* newFolderBtn = new QPushButton(QStringLiteral("+ Folder"), this);
    newFolderBtn->setObjectName(QStringLiteral("SmallButton"));
    connect(newFolderBtn, &QPushButton::clicked, this, &Sidebar::newFolderRequested);
    connect(m_folderList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit sectionSelected(SidebarSection::Folder, item->data(Qt::UserRole).toLongLong());
    });

    auto* tagLabel = new QLabel(QStringLiteral("TAGS"), this);
    tagLabel->setObjectName(QStringLiteral("SectionLabel"));

    m_tagList = new QListWidget(this);
    m_tagList->setObjectName(QStringLiteral("TagList"));
    connect(m_tagList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit sectionSelected(SidebarSection::Tag, item->data(Qt::UserRole).toLongLong());
    });

    layout->addWidget(brand);
    layout->addWidget(m_navList);
    layout->addWidget(folderLabel);
    layout->addWidget(m_folderList, 1);
    layout->addWidget(newFolderBtn);
    layout->addWidget(tagLabel);
    layout->addWidget(m_tagList, 1);

    m_navList->setCurrentRow(0);
}

void Sidebar::setFolders(const QVector<Folder>& folders) {
    m_folders = folders;
    rebuildFolderList();
}

void Sidebar::setTags(const QVector<Tag>& tags) {
    m_tags = tags;
    rebuildTagList();
}

void Sidebar::rebuildFolderList() {
    m_folderList->clear();
    for (const Folder& f : m_folders) {
        auto* item = new QListWidgetItem(QStringLiteral("▸ ") + f.name);
        item->setData(Qt::UserRole, f.id);
        m_folderList->addItem(item);
    }
}

void Sidebar::rebuildTagList() {
    m_tagList->clear();
    for (const Tag& t : m_tags) {
        auto* item = new QListWidgetItem(QStringLiteral("#") + t.name);
        item->setData(Qt::UserRole, t.id);
        m_tagList->addItem(item);
    }
}

void Sidebar::onNavItemClicked(int row) {
    switch (row) {
    case 0: emit sectionSelected(SidebarSection::AllNotes); break;
    case 1: emit sectionSelected(SidebarSection::Favorites); break;
    case 2: emit sectionSelected(SidebarSection::Recent); break;
    case 3: emit sectionSelected(SidebarSection::Settings); break;
    case 4: emit lockRequested(); break;
    default: break;
    }
}
