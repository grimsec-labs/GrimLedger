#pragma once

#include <QWidget>

class QContextMenuEvent;
class QListWidgetItem;
#include <QVector>
#include "models/Note.h"
#include "storage/NoteRepository.h"

class QLineEdit;
class QComboBox;
class QListWidget;
class QPushButton;

class NoteList : public QWidget {
    Q_OBJECT

public:
    explicit NoteList(QWidget* parent = nullptr);

    void setNotes(const QVector<Note>& notes);
    void selectNote(qint64 id);
    void clearSelection();
    void clearSearch();
    qint64 currentNoteId() const;

signals:
    void noteSelected(qint64 id);
    void newNoteRequested();
    void deleteNoteRequested(qint64 id);
    void duplicateNoteRequested(qint64 id);
    void sortChanged(NoteSortField field, bool descending);
    void searchChanged(const QString& query);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onSortChanged(int index);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void buildUi();

    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_sortCombo = nullptr;
    QListWidget* m_list = nullptr;
    QPushButton* m_newButton = nullptr;
    qint64 m_currentId = 0;
};
