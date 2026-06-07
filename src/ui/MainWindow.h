#pragma once

#include <QMainWindow>
#include <QVector>
#include <QSplitter>
#include <QComboBox>
#include <QLabel>
#include <QStatusBar>
#include <QStackedWidget>
#include <QHash>
#include <QTimer>
#include <memory>
#include "models/Note.h"
#include "models/Credential.h"
#include "models/CredentialSummary.h"
#include "storage/NoteRepository.h"
#include "storage/AttachmentRepository.h"
#include "markdown/MarkdownRenderer.h"
#include "ui/Sidebar.h"
#include "bridge/CredentialBridgeServer.h"
#include "bridge/BridgeFillCoordinator.h"
#include "bridge/BridgeClipCoordinator.h"
#include "security/ChamberManager.h"

class Sidebar;
class NoteList;
class NoteEditor;
class MarkdownPreview;
class CredentialList;
class CredentialEditor;
class SettingsWindow;
class VaultSession;
class Database;
class NoteRepository;
class CredentialRepository;
class VaultRepository;
class SealedBlockRepository;
class SecurityChronicle;
class RunbookSessionRepository;
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

    void onVaultUnlocked();
    void stopBridge();

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

    void onCredentialSelected(qint64 id);
    void onNewCredential();
    void onSaveCredential();
    void onDeleteCredential(qint64 id);
    void onCredentialSearchChanged(const QString& query);
    void onGenerateCredentialPassword();
    void onCopyCredentialPassword();
    void onCopyCredentialUsername();
    void onCopyCredentialTotp();
    void onCheckCredentialBreach();
    void onImportCredentials();
    void refreshVaultHealth();
    void onEnableHelloUnlock();
    void onDisableHelloUnlock();
    void onScanSecrets();
    void onRedactExport();
    void onStartRunbook();
    void onShowKnowledgeGraph();
    void onShowChronicle();
    void onExportGrimShare();
    void onImportGrimShare();
    void onLockWorkChamber();
    void onUnlockWorkChamber();

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
    void showNotesMode();
    void showPasswordsMode();
    void loadCredentials();
    void loadCredential(qint64 id);
    bool saveCurrentCredential(bool showErrorDialog = true);
    void applyAccent(const QString& hex);
    void updateStatusBar();
    void updatePreview();
    MarkdownRenderer::ImageUrlResolver imageResolver() const;
    void startBridge();
    void clearAllSensitiveState();
    void finalizeVaultRestore(bool cleanupWarning, const QString& warningDetail = QString());

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

    Database& m_db;
    VaultSession& m_session;
    AttachmentRepository m_attachments;
    std::unique_ptr<NoteRepository> m_notes;
    std::unique_ptr<CredentialRepository> m_credentials;
    std::unique_ptr<VaultRepository> m_vault;
    std::unique_ptr<BridgeFillCoordinator> m_fillCoordinator;
    std::unique_ptr<BridgeClipCoordinator> m_clipCoordinator;
    std::unique_ptr<CredentialBridgeServer> m_bridge;
    std::unique_ptr<SealedBlockRepository> m_sealedBlocks;
    std::unique_ptr<SecurityChronicle> m_chronicle;
    std::unique_ptr<RunbookSessionRepository> m_runbooks;
    ChamberManager m_chambers;

    Sidebar* m_sidebar = nullptr;
    QStackedWidget* m_listStack = nullptr;
    NoteList* m_noteList = nullptr;
    CredentialList* m_credList = nullptr;
    QStackedWidget* m_contentStack = nullptr;
    NoteEditor* m_editor = nullptr;
    MarkdownPreview* m_preview = nullptr;
    QWidget* m_credPanel = nullptr;
    CredentialEditor* m_credEditor = nullptr;
    SettingsWindow* m_settings = nullptr;
    CustomTitleBar* m_titleBar = nullptr;

    QWidget* m_mainPanel = nullptr;
    SettingsWindow* m_settingsPanel = nullptr;
    QSplitter* m_editorSplitter = nullptr;
    QComboBox* m_viewModeCombo = nullptr;
    QWidget* m_viewModeRow = nullptr;
    QStatusBar* m_status = nullptr;
    QLabel* m_wordLabel = nullptr;
    QLabel* m_charLabel = nullptr;
    QLabel* m_savedLabel = nullptr;
    QLabel* m_encryptLabel = nullptr;
    QLabel* m_autolockLabel = nullptr;
    QLabel* m_emptyEditorLabel = nullptr;

    QVector<Note> m_cachedNotes;
    QVector<CredentialSummary> m_cachedCredentials;
    QLabel* m_bridgeLabel = nullptr;
    qint64 m_currentNoteId = 0;
    qint64 m_currentCredentialId = 0;
    qint64 m_currentFolderId = 0;
    SidebarSection m_section = SidebarSection::AllNotes;
    qint64 m_filterId = 0;
    NoteSortField m_sortField = NoteSortField::Modified;
    bool m_sortDescending = true;
    QString m_searchQuery;
    QString m_credSearchQuery;
    QString m_accent;
    bool m_lockingVault = false;
    QTimer* m_previewTimer = nullptr;
    mutable QHash<QString, QString> m_attachmentPreviewCache;
};
