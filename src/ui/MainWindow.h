#pragma once

#include <QMainWindow>
#include <QVector>
#include <QSplitter>
#include <QComboBox>
#include <QLabel>
#include <QStatusBar>
#include <memory>
#include "models/Note.h"
#include "storage/NoteRepository.h"
#include "storage/AttachmentRepository.h"
#include "markdown/MarkdownRenderer.h"
#include "ui/Sidebar.h"

class Sidebar;
class NoteList;
class NoteEditor;
class MarkdownPreview;
class SettingsWindow;
class VaultSession;
class Database;
class NoteRepository;
class VaultRepository;
class CustomTitleBar;

enum class EditorViewMode;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(
        Database& db,
        VaultSession& session,
        QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSectionSelected(SidebarSection section, qint64 id);
    void onNoteSelected(qint64 id);
    void onNewNote();
    void onSaveNote();
    void onSaveAndClose();
    void onEditorChanged();
    void onSearchChanged(const QString& query);
    void onSortChanged(NoteSortField field, bool descending);
    void onLockVault();
    void onViewModeChanged(int index);
    void onFavoriteToggled(bool favorited);
    void onNewFolder();
    void refreshSidebar();
    void showSettings();
    void hideSettings();

    void onAccentChanged(const QString& hex);
    void onChangePassword(const QString& current, const QString& newPass);
    void onBackupVault(const QString& path);
    void onRestoreVault(const QString& path);
    void onImportMarkdown();
    void onExportNote();
    void onExportAllMarkdown();
    void onExportEncryptedArchive(const QString& path);
    void onInsertImage();

signals:
    void vaultLocked();

private:
    void buildUi();
    void loadNotes();
    void loadNote(qint64 id);
    void selectInitialNote();
    bool saveCurrentNote(bool showFolderPicker, bool closeAfter, bool showErrorDialog = true);
    void closeNoteEditor();
    void openNoteEditor();
    void applyAccent(const QString& hex);
    void updateStatusBar();
    void updatePreview();
    MarkdownRenderer::ImageUrlResolver imageResolver() const;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

    Database& m_db;
    VaultSession& m_session;
    AttachmentRepository m_attachments;
    std::unique_ptr<NoteRepository> m_notes;
    std::unique_ptr<VaultRepository> m_vault;

    Sidebar* m_sidebar = nullptr;
    NoteList* m_noteList = nullptr;
    NoteEditor* m_editor = nullptr;
    MarkdownPreview* m_preview = nullptr;
    SettingsWindow* m_settings = nullptr;
    CustomTitleBar* m_titleBar = nullptr;

    QWidget* m_mainPanel = nullptr;
    SettingsWindow* m_settingsPanel = nullptr;
    QSplitter* m_editorSplitter = nullptr;
    QComboBox* m_viewModeCombo = nullptr;
    QStatusBar* m_status = nullptr;
    QLabel* m_wordLabel = nullptr;
    QLabel* m_charLabel = nullptr;
    QLabel* m_savedLabel = nullptr;
    QLabel* m_encryptLabel = nullptr;
    QLabel* m_autolockLabel = nullptr;
    QLabel* m_emptyEditorLabel = nullptr;

    QVector<Note> m_cachedNotes;
    qint64 m_currentNoteId = 0;
    qint64 m_currentFolderId = 0;
    SidebarSection m_section = SidebarSection::AllNotes;
    qint64 m_filterId = 0;
    NoteSortField m_sortField = NoteSortField::Modified;
    bool m_sortDescending = true;
    QString m_searchQuery;
    QString m_accent;
    bool m_lockingVault = false;
};
