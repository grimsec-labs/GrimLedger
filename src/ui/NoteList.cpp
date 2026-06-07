#include "ui/NoteList.h"
#include "utils/TimeUtils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QContextMenuEvent>
#include <QListWidgetItem>

NoteList::NoteList(QWidget* parent)
    : QWidget(parent) {
    buildUi();
}

void NoteList::buildUi() {
    setObjectName(QStringLiteral("NoteListPanel"));
    setMinimumWidth(260);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName(QStringLiteral("SearchField"));
    m_searchEdit->setPlaceholderText(QStringLiteral("Search notes, tags, bodies..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &NoteList::searchChanged);

    m_sortCombo = new QComboBox(this);
    m_sortCombo->setObjectName(QStringLiteral("SortCombo"));
    m_sortCombo->addItem(QStringLiteral("Modified (newest)"), static_cast<int>(NoteSortField::Modified));
    m_sortCombo->addItem(QStringLiteral("Modified (oldest)"), static_cast<int>(NoteSortField::Modified) | 0x100);
    m_sortCombo->addItem(QStringLiteral("Title A-Z"), static_cast<int>(NoteSortField::Title));
    m_sortCombo->addItem(QStringLiteral("Title Z-A"), static_cast<int>(NoteSortField::Title) | 0x100);
    m_sortCombo->addItem(QStringLiteral("Created"), static_cast<int>(NoteSortField::Created));
    m_sortCombo->addItem(QStringLiteral("Favorites first"), static_cast<int>(NoteSortField::Favorite));
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NoteList::onSortChanged);

    m_newButton = new QPushButton(QStringLiteral("+ New Note"), this);
    m_newButton->setObjectName(QStringLiteral("PrimaryButton"));
    connect(m_newButton, &QPushButton::clicked, this, &NoteList::newNoteRequested);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("NoteListWidget"));
    connect(m_list, &QListWidget::itemClicked, this, &NoteList::onItemClicked);

    layout->addWidget(m_searchEdit);
    layout->addWidget(m_sortCombo);
    layout->addWidget(m_newButton);
    layout->addWidget(m_list, 1);
}

void NoteList::setNotes(const QVector<Note>& notes) {
    m_list->clear();
    for (const Note& n : notes) {
        QString label = n.title.isEmpty() ? QStringLiteral("(Untitled)") : n.title;
        if (n.isFavorite) {
            label = QStringLiteral("★ ") + label;
        }
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, n.id);
        item->setToolTip(TimeUtils::formatTimestamp(n.updatedAt));
        m_list->addItem(item);
    }
}

void NoteList::selectNote(qint64 id) {
    for (int i = 0; i < m_list->count(); ++i) {
        auto* item = m_list->item(i);
        if (item && item->data(Qt::UserRole).toLongLong() == id) {
            m_list->setCurrentRow(i);
            m_currentId = id;
            break;
        }
    }
}

qint64 NoteList::currentNoteId() const {
    return m_currentId;
}

void NoteList::onItemClicked(QListWidgetItem* item) {
    m_currentId = item->data(Qt::UserRole).toLongLong();
    emit noteSelected(m_currentId);
}

void NoteList::contextMenuEvent(QContextMenuEvent* event) {
    const auto* item = m_list->itemAt(m_list->mapFromGlobal(event->globalPos()));
    if (!item) return;

    const qint64 id = item->data(Qt::UserRole).toLongLong();
    QMenu menu(this);
    menu.addAction(QStringLiteral("Duplicate"), this, [this, id]() {
        emit duplicateNoteRequested(id);
    });
    menu.addAction(QStringLiteral("Delete"), this, [this, id]() {
        emit deleteNoteRequested(id);
    });
    menu.exec(event->globalPos());
}

void NoteList::onSortChanged(int index) {
    const int data = m_sortCombo->itemData(index).toInt();
    const bool descending = (data & 0x100) == 0;
    const auto field = static_cast<NoteSortField>(data & 0xFF);
    emit sortChanged(field, descending);
}
