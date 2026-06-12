#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/NoteList.h"
#include "ui/NoteEditor.h"
#include "ui/MarkdownPreview.h"
#include "ui/CredentialList.h"
#include "ui/CredentialEditor.h"
#include "ui/SettingsWindow.h"
#include "storage/CredentialRepository.h"
#include "utils/PasswordGenerator.h"
#include "utils/ClipboardUtils.h"
#include "ui/CustomTitleBar.h"
#include "ui/FramelessResize.h"
#include "ui/FolderPickerDialog.h"
#include "storage/VaultRepository.h"
#include "storage/NoteRepository.h"
#include "storage/AttachmentRepository.h"
#include "utils/ImageSanitizer.h"
#include "search/SearchEngine.h"
#include "models/Tag.h"
#include "security/PasswordManager.h"
#include "utils/TimeUtils.h"
#include "utils/Theme.h"
#include "utils/DialogUtils.h"
#include "utils/AppSettings.h"
#include "ui/GrimInputDialog.h"
#include "utils/SecurityLimits.h"
#include "utils/PathSafety.h"
#include "ui/GrimFileDialog.h"
#include "security/CryptoManager.h"
#include "security/VaultSession.h"
#include "security/BreachCheck.h"
#include "security/PlatformBiometricUnlock.h"
#include "import/CredentialImport.h"
#include "utils/TotpGenerator.h"
#include "utils/VaultHealth.h"
#include "storage/SealedBlockRepository.h"
#include "security/SecurityChronicle.h"
#include "security/SecretScanner.h"
#include "security/RedactionStudio.h"
#include "security/ChamberManager.h"
#include "models/ChamberId.h"
#include "runbook/RunbookParser.h"
#include "runbook/RunbookSessionRepository.h"
#include "graph/KnowledgeGraph.h"
#include "share/GrimShare.h"
#include "search/SemanticSearch.h"

#include <QLocalSocket>

#include <QSplitter>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QEvent>
#include <QAction>
#include <QKeySequence>
#include <QApplication>
#include <QDialog>

#include <algorithm>

MainWindow::MainWindow(Database& db, VaultSession& session, QWidget* parent)
    : QMainWindow(parent)
    , m_db(db)
    , m_session(session)
    , m_accent(Theme::savedAccent())
    , m_attachments(db) {
    m_notes = std::make_unique<NoteRepository>(m_db);
    m_credentials = std::make_unique<CredentialRepository>(m_db);
    m_vault = std::make_unique<VaultRepository>(m_db);
    m_fillCoordinator = std::make_unique<BridgeFillCoordinator>(this);
    m_clipCoordinator = std::make_unique<BridgeClipCoordinator>(this);
    m_sealedBlocks = std::make_unique<SealedBlockRepository>(m_db);
    m_chronicle = std::make_unique<SecurityChronicle>(m_db);
    m_runbooks = std::make_unique<RunbookSessionRepository>(m_db);
    m_notes->ensureDefaultFolder();
    buildUi();
    loadNotes();
    refreshSidebar();
    selectInitialNote();

    connect(&m_session, &VaultSession::activityReset, this, [this]() {
        m_autolockLabel->setText(
            QStringLiteral("Auto-lock: %1 min").arg(m_session.autoLockMinutes()));
    });
    connect(&m_session, &VaultSession::lockRequested, this, &MainWindow::onLockVault);
    m_session.setAutoLockEnabled(AppSettings::autoLockEnabled());
    m_session.setAutoLockMinutes(AppSettings::autoLockMinutes());

    m_previewTimer = new QTimer(this);
    m_previewTimer->setInterval(400);
    connect(m_previewTimer, &QTimer::timeout, this, &MainWindow::updatePreview);
    m_previewTimer->start();
}

MainWindow::~MainWindow() {
    stopBridge();
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (FramelessResize::handleNativeEvent(this, eventType, message, result)) {
        return true;
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::buildUi() {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowTitle(QStringLiteral("GrimLedger — Encrypted Markdown Vault"));
    setMinimumSize(1100, 860);
    resize(1280, 920);
    setObjectName(QStringLiteral("MainWindow"));

    auto* shell = new QWidget(this);
    auto* shellLayout = new QVBoxLayout(shell);
    shellLayout->setContentsMargins(0, 0, 0, 0);
    shellLayout->setSpacing(0);

    m_titleBar = new CustomTitleBar(shell);
    m_titleBar->setTitle(QStringLiteral("GRIMLEDGER"));
    m_titleBar->setSubtitle(QStringLiteral("// encrypted markdown vault"));

    auto* central = new QWidget(shell);
    auto* rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_sidebar = new Sidebar(this);
    connect(m_sidebar, &Sidebar::sectionSelected, this, &MainWindow::onSectionSelected);
    connect(m_sidebar, &Sidebar::lockRequested, this, &MainWindow::onLockVault);
    connect(m_sidebar, &Sidebar::newFolderRequested, this, &MainWindow::onNewFolder);

    m_noteList = new NoteList(this);
    connect(m_noteList, &NoteList::noteSelected, this, &MainWindow::onNoteSelected);
    connect(m_noteList, &NoteList::newNoteRequested, this, &MainWindow::onNewNote);
    connect(m_noteList, &NoteList::deleteNoteRequested, this, [this](qint64 id) {
        if (DialogUtils::question(this, QStringLiteral("Delete"),
                QStringLiteral("Delete this note permanently?"))) {
            m_notes->deleteNote(id);
            if (m_currentNoteId == id) {
                m_currentNoteId = 0;
                m_editor->setTitle(QString());
                m_editor->setBody(QString());
            }
            loadNotes();
            refreshSidebar();
        }
    });
    connect(m_noteList, &NoteList::duplicateNoteRequested, this, [this](qint64 id) {
        const qint64 newId = m_notes->duplicateNoteWithAttachments(
            id,
            m_session.key(),
            m_attachments);
        if (newId <= 0) {
            return;
        }

        loadNotes();
        loadNote(newId);
    });
    connect(m_noteList, &NoteList::searchChanged, this, &MainWindow::onSearchChanged);
    connect(m_noteList, &NoteList::sortChanged, this, &MainWindow::onSortChanged);

    m_mainPanel = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(m_mainPanel);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    m_viewModeRow = new QWidget(this);
    auto* viewRow = new QHBoxLayout(m_viewModeRow);
    viewRow->setContentsMargins(0, 0, 0, 0);
    viewRow->addStretch();
    m_viewModeCombo = new QComboBox(this);
    m_viewModeCombo->addItem(QStringLiteral("Split View"));
    m_viewModeCombo->addItem(QStringLiteral("Editor Only"));
    m_viewModeCombo->addItem(QStringLiteral("Preview Only"));
    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onViewModeChanged);
    viewRow->addWidget(new QLabel(QStringLiteral("View:"), this));
    viewRow->addWidget(m_viewModeCombo);

    m_editorSplitter = new QSplitter(Qt::Horizontal, this);
    m_editorSplitter->setObjectName(QStringLiteral("EditorPreviewSplitter"));
    m_editorSplitter->setHandleWidth(8);
    m_editorSplitter->setChildrenCollapsible(false);
    m_editor = new NoteEditor(this);
    m_preview = new MarkdownPreview(this);
    m_editorSplitter->addWidget(m_editor);
    m_editorSplitter->addWidget(m_preview);
    m_editorSplitter->setStretchFactor(0, 1);
    m_editorSplitter->setStretchFactor(1, 1);
    m_editorSplitter->setSizes({500, 500});

    connect(m_editor, &NoteEditor::contentChanged, this, &MainWindow::onEditorChanged);
    connect(m_editor, &NoteEditor::saveRequested, this, &MainWindow::onSaveNote);
    connect(m_editor, &NoteEditor::saveAndCloseRequested, this, &MainWindow::onSaveAndClose);
    connect(m_editor, &NoteEditor::favoriteToggled, this, &MainWindow::onFavoriteToggled);
    connect(m_editor, &NoteEditor::imageInsertRequested, this, &MainWindow::onInsertImage);

    m_emptyEditorLabel = new QLabel(
        QStringLiteral("◈ Select a note from the list, or click + New Note to begin."), this);
    m_emptyEditorLabel->setObjectName(QStringLiteral("EmptyEditor"));
    m_emptyEditorLabel->setAlignment(Qt::AlignCenter);
    m_emptyEditorLabel->setWordWrap(true);

    m_credEditor = new CredentialEditor(this);
    m_credPanel = new QWidget(this);
    auto* credLayout = new QVBoxLayout(m_credPanel);
    credLayout->setContentsMargins(0, 0, 0, 0);
    credLayout->addWidget(m_credEditor);

    connect(m_credEditor, &CredentialEditor::contentChanged, this, [this]() {
        m_session.resetActivityTimer();
    });
    connect(m_credEditor, &CredentialEditor::saveRequested, this, &MainWindow::onSaveCredential);
    connect(m_credEditor, &CredentialEditor::deleteRequested, this, [this]() {
        if (m_currentCredentialId > 0) {
            onDeleteCredential(m_currentCredentialId);
        }
    });
    connect(m_credEditor, &CredentialEditor::generatePasswordRequested,
            this, &MainWindow::onGenerateCredentialPassword);
    connect(m_credEditor, &CredentialEditor::copyPasswordRequested,
            this, &MainWindow::onCopyCredentialPassword);
    connect(m_credEditor, &CredentialEditor::copyUsernameRequested,
            this, &MainWindow::onCopyCredentialUsername);
    connect(m_credEditor, &CredentialEditor::copyTotpRequested,
            this, &MainWindow::onCopyCredentialTotp);
    connect(m_credEditor, &CredentialEditor::checkBreachRequested,
            this, &MainWindow::onCheckCredentialBreach);

    m_credList = new CredentialList(this);
    connect(m_credList, &CredentialList::credentialSelected, this, &MainWindow::onCredentialSelected);
    connect(m_credList, &CredentialList::newCredentialRequested, this, &MainWindow::onNewCredential);
    connect(m_credList, &CredentialList::searchChanged, this, &MainWindow::onCredentialSearchChanged);

    m_listStack = new QStackedWidget(this);
    m_listStack->addWidget(m_noteList);
    m_listStack->addWidget(m_credList);

    m_contentStack = new QStackedWidget(this);
    m_contentStack->addWidget(m_mainPanel);
    m_contentStack->addWidget(m_credPanel);

    mainLayout->addWidget(m_viewModeRow);
    mainLayout->addWidget(m_editorSplitter, 1);
    mainLayout->addWidget(m_emptyEditorLabel, 1);
    m_emptyEditorLabel->hide();

    m_settingsPanel = new SettingsWindow(this);
    m_settingsPanel->setAccentColor(m_accent);
    connect(m_settingsPanel, &SettingsWindow::backRequested, this, &MainWindow::hideSettings);
    connect(m_settingsPanel, &SettingsWindow::accentPreviewChanged, this, &MainWindow::onAccentPreviewChanged);
    connect(m_settingsPanel, &SettingsWindow::accentChanged, this, &MainWindow::onAccentChanged);
    connect(m_settingsPanel, &SettingsWindow::lineNumbersChanged, m_editor, &NoteEditor::setLineNumbersVisible);
    connect(m_settingsPanel, &SettingsWindow::wordWrapChanged, m_editor, &NoteEditor::setWordWrapEnabled);
    connect(m_settingsPanel, &SettingsWindow::autoLockChanged, this, [this](bool on, int min) {
        m_session.setAutoLockEnabled(on);
        m_session.setAutoLockMinutes(min);
        m_autolockLabel->setText(on
            ? QStringLiteral("Auto-lock: %1 min").arg(min)
            : QStringLiteral("Auto-lock: off"));
    });
    connect(m_settingsPanel, &SettingsWindow::browserBridgeChanged, this, [this](bool on) {
        if (on) {
            startBridge();
        } else {
            stopBridge();
        }
        if (m_bridgeLabel) {
            m_bridgeLabel->setText(on && m_bridge
                ? QStringLiteral("Bridge: on")
                : QStringLiteral("Bridge: off"));
        }
    });
    connect(m_settingsPanel, &SettingsWindow::changePasswordRequested,
            this, &MainWindow::onChangePassword);
    connect(m_settingsPanel, &SettingsWindow::backupVaultRequested, this, &MainWindow::onBackupVault);
    connect(m_settingsPanel, &SettingsWindow::restoreVaultRequested, this, &MainWindow::onRestoreVault);
    connect(m_settingsPanel, &SettingsWindow::importMarkdownRequested, this, &MainWindow::onImportMarkdown);
    connect(m_settingsPanel, &SettingsWindow::exportNoteRequested, this, &MainWindow::onExportNote);
    connect(m_settingsPanel, &SettingsWindow::exportAllMarkdownRequested, this, &MainWindow::onExportAllMarkdown);
    connect(m_settingsPanel, &SettingsWindow::exportEncryptedArchiveRequested,
            this, &MainWindow::onExportEncryptedArchive);
    connect(m_settingsPanel, &SettingsWindow::importCredentialsRequested,
            this, &MainWindow::onImportCredentials);
    connect(m_settingsPanel, &SettingsWindow::refreshHealthRequested,
            this, &MainWindow::refreshVaultHealth);
    connect(m_settingsPanel, &SettingsWindow::enableHelloUnlockRequested,
            this, &MainWindow::onEnableHelloUnlock);
    connect(m_settingsPanel, &SettingsWindow::disableHelloUnlockRequested,
            this, &MainWindow::onDisableHelloUnlock);
    connect(m_settingsPanel, &SettingsWindow::scanSecretsRequested, this, &MainWindow::onScanSecrets);
    connect(m_settingsPanel, &SettingsWindow::redactExportRequested, this, &MainWindow::onRedactExport);
    connect(m_settingsPanel, &SettingsWindow::startRunbookRequested, this, &MainWindow::onStartRunbook);
    connect(m_settingsPanel, &SettingsWindow::showKnowledgeGraphRequested, this, &MainWindow::onShowKnowledgeGraph);
    connect(m_settingsPanel, &SettingsWindow::showChronicleRequested, this, &MainWindow::onShowChronicle);
    connect(m_settingsPanel, &SettingsWindow::exportGrimShareRequested, this, &MainWindow::onExportGrimShare);
    connect(m_settingsPanel, &SettingsWindow::importGrimShareRequested, this, &MainWindow::onImportGrimShare);
    connect(m_settingsPanel, &SettingsWindow::lockWorkChamberRequested, this, &MainWindow::onLockWorkChamber);
    connect(m_settingsPanel, &SettingsWindow::unlockWorkChamberRequested, this, &MainWindow::onUnlockWorkChamber);

    m_rightStack = new QStackedWidget(this);
    m_rightStack->addWidget(m_contentStack);
    m_rightStack->addWidget(m_settingsPanel);

    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    rightLayout->addWidget(m_rightStack, 1);

    rootLayout->addWidget(m_sidebar);
    rootLayout->addWidget(m_listStack);
    rootLayout->addWidget(rightWidget, 1);

    shellLayout->addWidget(m_titleBar);
    shellLayout->addWidget(central, 1);
    setCentralWidget(shell);

    m_status = statusBar();
    m_wordLabel = new QLabel(QStringLiteral("Words: 0"), this);
    m_charLabel = new QLabel(QStringLiteral("Chars: 0"), this);
    m_savedLabel = new QLabel(QStringLiteral("Last saved: —"), this);
    m_encryptLabel = new QLabel(QStringLiteral("🔒 Vault Encrypted"), this);
    m_autolockLabel = new QLabel(
        m_session.autoLockEnabled()
            ? QStringLiteral("Auto-lock: %1 min").arg(m_session.autoLockMinutes())
            : QStringLiteral("Auto-lock: off"),
        this);
    m_bridgeLabel = new QLabel(QStringLiteral("Bridge: off"), this);

    auto* lockBtn = new QPushButton(QStringLiteral("Lock"), this);
    lockBtn->setObjectName(QStringLiteral("SmallButton"));
    connect(lockBtn, &QPushButton::clicked, this, &MainWindow::onLockVault);

    m_status->addWidget(m_wordLabel);
    m_status->addWidget(new QLabel(QStringLiteral(" | "), this));
    m_status->addWidget(m_charLabel);
    m_status->addWidget(new QLabel(QStringLiteral(" | "), this));
    m_status->addWidget(m_savedLabel);
    m_status->addPermanentWidget(m_encryptLabel);
    m_status->addPermanentWidget(m_bridgeLabel);
    m_status->addPermanentWidget(m_autolockLabel);
    m_status->addPermanentWidget(lockBtn);

    auto* saveAction = new QAction(this);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveAndClose);
    addAction(saveAction);

    applyAccent(m_accent);
    m_editor->setLineNumbersVisible(AppSettings::lineNumbersEnabled());
    m_editor->setWordWrapEnabled(AppSettings::wordWrapEnabled());
    qApp->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (m_session.isUnlocked()) {
        switch (event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::KeyPress:
            m_session.resetActivityTimer();
            break;
        default:
            break;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::loadNotes() {
    const QByteArray& key = m_session.key();

    bool favoritesOnly = false;
    qint64 folderId = -1;
    int recentLimit = -1;

    switch (m_section) {
    case SidebarSection::Favorites:
        favoritesOnly = true;
        break;
    case SidebarSection::Recent:
        recentLimit = 20;
        break;
    case SidebarSection::Folder:
        folderId = m_filterId;
        break;
    default:
        break;
    }

    m_cachedNotes = m_notes->listNotes(
        key, m_sortField, m_sortDescending, folderId, favoritesOnly, recentLimit);

    if (m_section == SidebarSection::Tag && m_filterId > 0) {
        const auto allTags = m_notes->listTags();
        QString tagName;
        for (const Tag& t : allTags) {
            if (t.id == m_filterId) {
                tagName = t.name;
                break;
            }
        }
        if (!tagName.isEmpty()) {
            QVector<Note> filtered;
            for (const Note& n : m_cachedNotes) {
                if (n.tags.contains(tagName)) {
                    filtered.append(n);
                }
            }
            m_cachedNotes = filtered;
        }
    }

    if (!m_searchQuery.trimmed().isEmpty()) {
        QHash<qint64, QString> bodies;
        for (const Note& n : m_cachedNotes) {
            const auto full = m_notes->getNote(n.id, key);
            if (full) {
                bodies.insert(
                    n.id,
                    m_sealedBlocks->strippedBodyForSearch(n.id, full->body));
            }
        }
        QVector<Note> filtered;
        if (AppSettings::semanticSearchEnabled()) {
            const auto matches = SemanticSearch::search(m_cachedNotes, bodies, m_searchQuery);
            for (const SemanticMatch& match : matches) {
                Note copy = match.note;
                if (bodies.contains(copy.id)) {
                    copy.body = bodies[copy.id];
                }
                filtered.append(copy);
            }
        } else {
            const auto matches = SearchEngine::search(m_cachedNotes, m_searchQuery, true, bodies);
            for (const SearchMatch& m : matches) {
                Note copy = m.note;
                if (bodies.contains(copy.id)) {
                    copy.body = bodies[copy.id];
                }
                filtered.append(copy);
            }
        }
        m_noteList->setNotes(filtered);
    } else {
        m_noteList->setNotes(m_cachedNotes);
    }
}

void MainWindow::selectInitialNote() {
    if (m_currentNoteId > 0) {
        return;
    }
    if (m_cachedNotes.isEmpty()) {
        onNewNote();
        return;
    }
    loadNote(m_cachedNotes.first().id);
    m_noteList->selectNote(m_currentNoteId);
}

void MainWindow::loadNote(qint64 id) {
    const auto note = m_notes->getNote(id, m_session.key());
    if (!note) return;

    m_currentNoteId = id;
    m_currentFolderId = note->folderId > 0
        ? note->folderId
        : m_notes->defaultFolderId();
    openNoteEditor();
    m_editor->setIntegrityError(note->integrityError);
    m_editor->setTitle(note->title);
    m_editor->setBody(note->body);
    m_editor->setTags(note->tags.join(QStringLiteral(", ")));
    m_editor->setFavorite(note->isFavorite);
    m_editor->setSavedState(true, note->updatedAt);
    updatePreview();
    updateStatusBar();
    m_session.resetActivityTimer();
}

void MainWindow::onSectionSelected(SidebarSection section, qint64 id) {
    if (section == SidebarSection::Settings) {
        showSettings();
        return;
    }
    if (section == SidebarSection::Lock) {
        onLockVault();
        return;
    }

    hideSettings();

    if (section == SidebarSection::Passwords) {
        saveCurrentNote(false, false, false);
        m_section = section;
        showPasswordsMode();
        loadCredentials();
        return;
    }

    saveCurrentCredential(false);
    showNotesMode();
    m_section = section;
    m_filterId = id;
    loadNotes();
}

void MainWindow::showNotesMode() {
    m_listStack->setCurrentWidget(m_noteList);
    m_contentStack->setCurrentWidget(m_mainPanel);
    m_viewModeRow->show();
}

void MainWindow::showPasswordsMode() {
    m_listStack->setCurrentWidget(m_credList);
    m_contentStack->setCurrentWidget(m_credPanel);
    m_viewModeRow->hide();
}

void MainWindow::loadCredentials() {
    const QByteArray& key = m_session.key();
    m_cachedCredentials = m_credSearchQuery.trimmed().isEmpty()
        ? m_credentials->listCredentialSummaries(key)
        : m_credentials->searchCredentialSummaries(m_credSearchQuery, key);
    m_credList->setCredentials(m_cachedCredentials);

    if (m_currentCredentialId > 0) {
        const bool stillExists = std::any_of(
            m_cachedCredentials.cbegin(),
            m_cachedCredentials.cend(),
            [this](const CredentialSummary& c) { return c.id == m_currentCredentialId; });
        if (stillExists) {
            m_credList->selectCredential(m_currentCredentialId);
            return;
        }
    }

    if (m_cachedCredentials.isEmpty()) {
        m_currentCredentialId = 0;
        m_credEditor->clearFields();
        return;
    }

    loadCredential(m_cachedCredentials.first().id);
    m_credList->selectCredential(m_currentCredentialId);
}

void MainWindow::loadCredential(qint64 id) {
    const auto cred = m_credentials->getCredential(id, m_session.key());
    if (!cred) {
        return;
    }

    m_currentCredentialId = id;
    m_credEditor->setIntegrityError(cred->integrityError);
    if (cred->integrityError) {
        m_credEditor->setLabel(cred->label);
        m_credEditor->setUsername(QString());
        m_credEditor->setPassword(QString());
        m_credEditor->setUrl(QString());
        m_credEditor->setNotes(QString());
        return;
    }

    m_credEditor->setLabel(cred->label);
    m_credEditor->setUsername(cred->username);
    m_credEditor->setPassword(cred->password);
    m_credEditor->setUrl(cred->url);
    m_credEditor->setNotes(cred->notes);
    m_credEditor->setTotpSecret(cred->totpSecret);
    m_credEditor->setFillTrustLevel(cred->fillTrustLevel);
    m_credEditor->setSavedState(true, cred->updatedAt);
    m_session.resetActivityTimer();
}

bool MainWindow::saveCurrentCredential(bool showErrorDialog) {
    if (m_section != SidebarSection::Passwords || m_currentCredentialId <= 0) {
        return true;
    }
    if (m_credEditor->integrityError()) {
        return true;
    }

    Credential c;
    c.id = m_currentCredentialId;
    c.label = m_credEditor->label();
    c.username = m_credEditor->username();
    c.password = m_credEditor->password();
    c.url = m_credEditor->url();
    c.notes = m_credEditor->notes();
    c.totpSecret = m_credEditor->totpSecret();
    c.fillTrustLevel = m_credEditor->fillTrustLevel();

    if (c.label.isEmpty()) {
        if (showErrorDialog) {
            DialogUtils::warning(
                this,
                QStringLiteral("Vault Key"),
                QStringLiteral("Enter a label for this credential."));
        }
        return false;
    }

    if (!m_credentials->updateCredential(c, m_session.key())) {
        if (showErrorDialog) {
            DialogUtils::warning(
                this,
                QStringLiteral("Vault Key"),
                QStringLiteral("Could not save credential."));
        }
        return false;
    }

    const auto saved = m_credentials->getCredential(c.id, m_session.key());
    if (saved) {
        m_credEditor->setSavedState(true, saved->updatedAt);
    }
    loadCredentials();
    return true;
}

void MainWindow::onCredentialSelected(qint64 id) {
    if (m_currentCredentialId > 0 && m_currentCredentialId != id) {
        if (!saveCurrentCredential(false)) {
            m_credList->selectCredential(m_currentCredentialId);
            return;
        }
    }
    loadCredential(id);
}

void MainWindow::onNewCredential() {
    Credential c;
    c.label = QStringLiteral("New Vault Key");
    c.username = QString();
    c.password = PasswordGenerator::generate();
    c.url = QString();
    c.notes = QString();

    const qint64 id = m_credentials->createCredential(c, m_session.key());
    if (id <= 0) {
        DialogUtils::warning(
            this,
            QStringLiteral("Vault Key"),
            QStringLiteral("Could not create credential."));
        return;
    }

    m_currentCredentialId = id;
    loadCredentials();
    loadCredential(id);
    m_credList->selectCredential(id);
}

void MainWindow::onSaveCredential() {
    saveCurrentCredential(true);
}

void MainWindow::onDeleteCredential(qint64 id) {
    if (!DialogUtils::question(
            this,
            QStringLiteral("Delete Vault Key"),
            QStringLiteral("Delete this credential permanently?"))) {
        return;
    }

    if (!m_credentials->deleteCredential(id)) {
        DialogUtils::warning(
            this,
            QStringLiteral("Delete Vault Key"),
            QStringLiteral("Could not delete credential."));
        return;
    }
    if (m_currentCredentialId == id) {
        m_currentCredentialId = 0;
        m_credEditor->clearFields();
    }
    loadCredentials();
}

void MainWindow::onCredentialSearchChanged(const QString& query) {
    m_credSearchQuery = query;
    loadCredentials();
}

void MainWindow::onGenerateCredentialPassword() {
    m_credEditor->setPassword(PasswordGenerator::generate());
    m_credEditor->setSavedState(false);
}

void MainWindow::onCopyCredentialPassword() {
    const QString pass = m_credEditor->password();
    if (pass.isEmpty()) {
        return;
    }
    ClipboardUtils::copyTextWithAutoClear(pass);
    DialogUtils::information(
        this,
        QStringLiteral("Copied"),
        QStringLiteral("Password copied. Clipboard clears in 20 seconds."));
}

void MainWindow::onCopyCredentialUsername() {
    const QString user = m_credEditor->username();
    if (user.isEmpty()) {
        return;
    }
    ClipboardUtils::copyTextWithAutoClear(user);
    DialogUtils::information(
        this,
        QStringLiteral("Copied"),
        QStringLiteral("Username copied. Clipboard clears in 20 seconds."));
}

void MainWindow::onCopyCredentialTotp() {
    const QString secret = m_credEditor->totpSecret();
    if (!TotpGenerator::isValidSecret(secret)) {
        DialogUtils::warning(
            this,
            QStringLiteral("TOTP"),
            QStringLiteral("Enter a valid base32 TOTP secret first."));
        return;
    }
    const QString code = TotpGenerator::currentCode(secret);
    ClipboardUtils::copyTextWithAutoClear(code);
    DialogUtils::information(
        this,
        QStringLiteral("Copied"),
        QStringLiteral("TOTP code copied. Clipboard clears in 20 seconds."));
}

void MainWindow::onCheckCredentialBreach() {
    if (!BreachCheck::isEnabled()) {
        DialogUtils::information(
            this,
            QStringLiteral("Breach Check"),
            QStringLiteral("Enable Have I Been Pwned checks in Vault Settings first."));
        return;
    }
    const QString pass = m_credEditor->password();
    if (pass.isEmpty()) {
        return;
    }
    const BreachCheck::Result result = BreachCheck::checkPassword(pass);
    if (!result.ok) {
        DialogUtils::warning(this, QStringLiteral("Breach Check"), result.error);
        return;
    }
    if (result.breached) {
        DialogUtils::warning(
            this,
            QStringLiteral("Breach Check"),
            QStringLiteral("This password appears in known breach data (%1 exposures).")
                .arg(result.count));
    } else {
        DialogUtils::information(
            this,
            QStringLiteral("Breach Check"),
            QStringLiteral("No known breach matches for this password."));
    }
}

void MainWindow::onImportCredentials() {
    const QString path = GrimFileDialog::getOpenFileName(
        this,
        QStringLiteral("Import Credentials"),
        QString(),
        QStringLiteral("Imports (*.csv *.xml);;Bitwarden CSV (*.csv);;KeePass XML (*.xml)"));
    if (path.isEmpty()) {
        return;
    }

    if (!DialogUtils::question(
            this,
            QStringLiteral("Import Credentials"),
            QStringLiteral("Imported data is plaintext in memory during import. Continue?"))) {
        return;
    }

    ImportParseResult parsed;
    if (path.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
        parsed = CredentialImport::parseKeePassXml(path);
    } else {
        parsed = CredentialImport::parseBitwardenCsv(path);
    }

    if (!parsed.ok) {
        DialogUtils::warning(this, QStringLiteral("Import"), parsed.error);
        return;
    }

    QVector<Credential> creds;
    creds.reserve(parsed.items.size());
    for (const ImportedCredential& item : parsed.items) {
        creds.append(item.credential);
    }

    int imported = 0;
    if (!m_credentials->importCredentials(creds, m_session.key(), &imported)) {
        DialogUtils::warning(
            this,
            QStringLiteral("Import"),
            QStringLiteral("Import failed. No credentials were added."));
        return;
    }

    loadCredentials();
    DialogUtils::information(
        this,
        QStringLiteral("Import"),
        QStringLiteral("Imported %1 credentials.").arg(imported));
}

void MainWindow::refreshVaultHealth() {
    int noteIntegrityErrors = 0;
    const QVector<Note> notes = m_notes->listNotes(m_session.key());
    for (const Note& note : notes) {
        if (note.integrityError) {
            ++noteIntegrityErrors;
        }
    }

    const VaultHealthReport report = VaultHealth::collect(
        m_db.path(),
        m_credentials->listCredentialSummaries(m_session.key()).size(),
        m_credentials->countCredentialIntegrityErrors(m_session.key()),
        notes.size(),
        noteIntegrityErrors,
        0,
        AppSettings::browserBridgeEnabled(),
        m_bridge && m_bridge->isListening(),
        !m_session.isUnlocked());
    m_settingsPanel->setVaultHealthReport(report);
}

void MainWindow::onEnableHelloUnlock() {
    if (!PlatformBiometricUnlock::isPlatformSupported()) {
        return;
    }
    bool ok = false;
    const QString password = GrimInputDialog::getPassword(
        this,
        QStringLiteral("Enable Quick Unlock"),
        QStringLiteral("Re-enter your master password to enable Quick Unlock:"),
        &ok);
    if (!ok || password.isEmpty()) {
        return;
    }

    QByteArray verifyKey;
    if (!m_vault->unlockVault(password, verifyKey)) {
        DialogUtils::warning(
            this,
            QStringLiteral("Quick Unlock"),
            QStringLiteral("Master password verification failed."));
        return;
    }

    if (!PlatformBiometricUnlock::enable(verifyKey, password)) {
        CryptoManager::secureZero(verifyKey);
        DialogUtils::warning(
            this,
            QStringLiteral("Quick Unlock"),
            PlatformBiometricUnlock::lastError());
        return;
    }
    CryptoManager::secureZero(verifyKey);

    DialogUtils::information(
        this,
        QStringLiteral("Quick Unlock"),
        QStringLiteral(
            "Quick Unlock enabled.\n\n"
            "This protects the vault key with your Windows user account (DPAPI). "
            "It does NOT require a separate Windows Hello / biometric gesture: anyone "
            "signed in to your Windows account can unlock the vault without the master "
            "password, so keep your account locked when away. Your master password "
            "remains available as the primary unlock."));
}

void MainWindow::onDisableHelloUnlock() {
    PlatformBiometricUnlock::disable();
    DialogUtils::information(
        this,
        QStringLiteral("Quick Unlock"),
        QStringLiteral("Quick Unlock disabled."));
}

void MainWindow::onScanSecrets() {
    int totalFindings = 0;
    const QByteArray key = m_session.key();
    for (const Note& summary : m_cachedNotes) {
        const auto note = m_notes->getNote(summary.id, key);
        if (!note) {
            continue;
        }
        const QString body = m_sealedBlocks->strippedBodyForSearch(note->id, note->body);
        totalFindings += SecretScanner::scanText(note->title + QLatin1Char('\n') + body).size();
    }
    DialogUtils::information(
        this,
        QStringLiteral("Secret Scanner"),
        totalFindings > 0
            ? QStringLiteral("Found %1 potential secret(s). Review notes and migrate to vault keys or sealed blocks.")
                .arg(totalFindings)
            : QStringLiteral("No likely secrets detected in unlocked notes."));
}

void MainWindow::onRedactExport() {
    if (m_currentNoteId <= 0) {
        DialogUtils::warning(this, QStringLiteral("Redaction"), QStringLiteral("Open a note first."));
        return;
    }
    const auto note = m_notes->getNote(m_currentNoteId, m_session.key());
    if (!note) {
        return;
    }
    const QString stripped = m_sealedBlocks->strippedBodyForExport(note->id, note->body);
    const QVector<RedactionHit> hits = RedactionStudio::detectHits(note->title + QLatin1Char('\n') + stripped);
    const QString redacted = RedactionStudio::applyRedactions(
        note->title + QStringLiteral("\n\n") + stripped, hits);
    const QString path = GrimFileDialog::getSaveFileName(
        this,
        QStringLiteral("Redacted Export"),
        QStringLiteral("redacted-note.md"),
        QStringLiteral("Markdown (*.md)"));
    if (path.isEmpty()) {
        return;
    }
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        DialogUtils::warning(this, QStringLiteral("Redaction"), QStringLiteral("Could not write file."));
        return;
    }
    out.write(redacted.toUtf8());
    if (m_chronicle) {
        m_chronicle->appendEvent(
            QStringLiteral("redacted_export"),
            QStringLiteral("Redacted export of note %1").arg(note->title),
            m_session.key());
    }
    DialogUtils::information(
        this,
        QStringLiteral("Redaction"),
        QStringLiteral("Redacted copy saved. Automatic redaction is not complete — review before sharing."));
}

void MainWindow::onStartRunbook() {
    if (m_currentNoteId <= 0) {
        DialogUtils::warning(this, QStringLiteral("Runbook"), QStringLiteral("Open a runbook note first."));
        return;
    }
    const auto note = m_notes->getNote(m_currentNoteId, m_session.key());
    if (!note || !RunbookParser::isRunbookNote(note->body)) {
        DialogUtils::warning(
            this,
            QStringLiteral("Runbook"),
            QStringLiteral("Current note has no checklist steps. Add `- [ ]` tasks or numbered steps."));
        return;
    }
    const QVector<RunbookStep> steps = RunbookParser::parseSteps(note->body);
    const qint64 sessionId = m_runbooks->createSession(note->id, steps.size(), m_session.key());
    if (sessionId <= 0) {
        DialogUtils::warning(this, QStringLiteral("Runbook"), QStringLiteral("Could not start session."));
        return;
    }
    DialogUtils::information(
        this,
        QStringLiteral("Runbook"),
        QStringLiteral("Runbook session started (%1 steps). Session id: %2")
            .arg(steps.size())
            .arg(sessionId));
}

void MainWindow::onShowKnowledgeGraph() {
    QHash<qint64, QString> bodies;
    const QByteArray key = m_session.key();
    for (const Note& n : m_cachedNotes) {
        if (const auto note = m_notes->getNote(n.id, key)) {
            bodies.insert(n.id, note->body);
        }
    }
    const KnowledgeGraphModel model = KnowledgeGraph::buildFromNotes(m_cachedNotes, bodies);
    const QString dot = KnowledgeGraph::toDot(model);
    const QString path = GrimFileDialog::getSaveFileName(
        this,
        QStringLiteral("Knowledge Graph"),
        QStringLiteral("grimledger-graph.dot"),
        QStringLiteral("Graphviz DOT (*.dot)"));
    if (path.isEmpty()) {
        return;
    }
    QFile out(path);
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        out.write(dot.toUtf8());
    }
    DialogUtils::information(
        this,
        QStringLiteral("Knowledge Graph"),
        QStringLiteral("Exported %1 nodes and %2 edges to DOT.")
            .arg(model.nodes.size())
            .arg(model.edges.size()));
}

void MainWindow::onShowChronicle() {
    const QVector<ChronicleEntry> entries = m_chronicle->listEntries(m_session.key(), 50);
    QString text;
    for (const ChronicleEntry& entry : entries) {
        text += QStringLiteral("[%1] %2 — %3\n")
            .arg(entry.createdAt.toString(Qt::ISODate), entry.eventType, entry.summary);
    }
    if (text.isEmpty()) {
        text = QStringLiteral("No security events recorded yet.");
    }
    const bool chainOk = m_chronicle->verifyChain(m_session.key());
    text += QStringLiteral("\nChain integrity: %1")
        .arg(chainOk ? QStringLiteral("OK") : QStringLiteral("CHECK FAILED"));
    DialogUtils::information(this, QStringLiteral("Grim Chronicle"), text);
}

void MainWindow::onExportGrimShare() {
    if (m_currentNoteId <= 0) {
        DialogUtils::warning(this, QStringLiteral("GrimShare"), QStringLiteral("Open a note to export."));
        return;
    }
    const auto note = m_notes->getNote(m_currentNoteId, m_session.key());
    if (!note) {
        return;
    }
    bool ok = false;
    const QString passphrase = GrimInputDialog::getPassword(
        this,
        QStringLiteral("GrimShare"),
        QStringLiteral("One-time passphrase for this package:"),
        &ok);
    if (!ok || passphrase.isEmpty()) {
        return;
    }
    const QString path = GrimFileDialog::getSaveFileName(
        this,
        QStringLiteral("Export GrimShare"),
        QStringLiteral("share.grimshare"),
        QStringLiteral("GrimShare (*.grimshare)"));
    if (path.isEmpty()) {
        return;
    }
    GrimShareNotePayload payload;
    payload.title = note->title;
    payload.body = m_sealedBlocks->strippedBodyForExport(note->id, note->body);
    payload.tags = note->tags;
    const GrimSharePackResult result = GrimShare::packNotes(path, {payload}, passphrase);
    if (!result.ok) {
        DialogUtils::warning(this, QStringLiteral("GrimShare"), result.error);
        return;
    }
    if (m_chronicle) {
        m_chronicle->appendEvent(
            QStringLiteral("grimshare_export"),
            QStringLiteral("Exported GrimShare package for \"%1\"").arg(note->title),
            m_session.key());
    }
    DialogUtils::information(this, QStringLiteral("GrimShare"), QStringLiteral("Package saved."));
}

void MainWindow::onImportGrimShare() {
    const QString path = GrimFileDialog::getOpenFileName(
        this,
        QStringLiteral("Import GrimShare"),
        QString(),
        QStringLiteral("GrimShare (*.grimshare)"));
    if (path.isEmpty()) {
        return;
    }
    bool ok = false;
    const QString passphrase = GrimInputDialog::getPassword(
        this,
        QStringLiteral("GrimShare"),
        QStringLiteral("Enter package passphrase:"),
        &ok);
    if (!ok || passphrase.isEmpty()) {
        return;
    }
    const GrimShareUnpackResult unpacked = GrimShare::unpackFile(path, passphrase);
    if (!unpacked.ok) {
        DialogUtils::warning(this, QStringLiteral("GrimShare"), unpacked.error);
        return;
    }
    if (!DialogUtils::question(
            this,
            QStringLiteral("GrimShare Import"),
            QStringLiteral("Import %1 note(s) into your vault?").arg(unpacked.notes.size()))) {
        return;
    }
    int imported = 0;
    for (const GrimShareNotePayload& payload : unpacked.notes) {
        Note note;
        note.title = payload.title.isEmpty() ? QStringLiteral("Imported Share") : payload.title;
        note.body = payload.body;
        note.folderId = m_notes->defaultFolderId();
        note.tags = payload.tags;
        if (m_notes->createNote(note, m_session.key()) > 0) {
            ++imported;
        }
    }
    loadNotes();
    DialogUtils::information(
        this,
        QStringLiteral("GrimShare"),
        QStringLiteral("Imported %1 note(s).").arg(imported));
}

void MainWindow::onLockWorkChamber() {
    m_chambers.lockChamber(ChamberId::Work);
    if (m_chronicle) {
        m_chronicle->appendEvent(
            QStringLiteral("chamber_lock"),
            QStringLiteral("Locked Work chamber"),
            m_session.key());
    }
    DialogUtils::information(
        this,
        QStringLiteral("Crypt Chambers"),
        QStringLiteral("Work chamber locked. Notes tagged for Work are hidden until re-unlocked."));
}

void MainWindow::onUnlockWorkChamber() {
    bool ok = false;
    const QString password = GrimInputDialog::getPassword(
        this,
        QStringLiteral("Unlock Work Chamber"),
        QStringLiteral("Re-enter master password to unlock Work chamber:"),
        &ok);
    if (!ok || password.isEmpty()) {
        return;
    }
    QByteArray key;
    if (!m_vault->unlockVault(password, key)) {
        DialogUtils::warning(this, QStringLiteral("Crypt Chambers"), QStringLiteral("Password incorrect."));
        return;
    }
    CryptoManager::secureZero(key);
    if (!m_chambers.unlockChamber(ChamberId::Work, m_session.key())) {
        DialogUtils::warning(this, QStringLiteral("Crypt Chambers"), QStringLiteral("Could not unlock chamber."));
        return;
    }
    if (m_chronicle) {
        m_chronicle->appendEvent(
            QStringLiteral("chamber_unlock"),
            QStringLiteral("Unlocked Work chamber"),
            m_session.key());
    }
    DialogUtils::information(this, QStringLiteral("Crypt Chambers"), QStringLiteral("Work chamber unlocked."));
}

void MainWindow::onNoteSelected(qint64 id) {
    if (m_currentNoteId > 0 && m_currentNoteId != id) {
        saveCurrentNote(false, false);
    }
    loadNote(id);
}

void MainWindow::onNewNote() {
    Note n;
    n.title = QStringLiteral("Untitled Note");
    n.body = QStringLiteral("# New Note\n\nBegin writing...");
    n.folderId = m_notes->defaultFolderId();
    const qint64 id = m_notes->createNote(n, m_session.key());
    if (id > 0) {
        loadNotes();
        loadNote(id);
        m_noteList->selectNote(id);
    }
}

void MainWindow::onSaveNote() {
    saveCurrentNote(false, false, false);
}

void MainWindow::onSaveAndClose() {
    saveCurrentNote(true, true, true);
}

bool MainWindow::saveCurrentNote(bool showFolderPicker, bool closeAfter, bool showErrorDialog) {
    if (m_editor && m_editor->integrityError()) {
        return true;
    }
    if (m_currentNoteId <= 0) {
        if (showFolderPicker) {
            DialogUtils::information(
                this,
                QStringLiteral("Save"),
                QStringLiteral("Open or create a note before saving."));
        }
        return false;
    }

    qint64 folderId = m_currentFolderId > 0
        ? m_currentFolderId
        : m_notes->defaultFolderId();
    if (folderId <= 0) {
        folderId = m_notes->ensureDefaultFolder();
    }

    if (showFolderPicker) {
        const auto folders = m_notes->listFolders();
        if (folders.isEmpty()) {
            DialogUtils::warning(
                this,
                QStringLiteral("Save"),
                QStringLiteral("No folders available. Create a folder first."));
            return false;
        }

        FolderPickerDialog dialog(
            folders,
            m_notes->defaultFolderId(),
            folderId,
            this);
        if (dialog.exec() != QDialog::Accepted) {
            return false;
        }

        folderId = dialog.selectedFolderId();
        if (folderId <= 0) {
            return false;
        }

        if (dialog.setAsDefaultRequested()) {
            m_notes->setDefaultFolderId(folderId);
        }
    }

    Note n;
    n.id = m_currentNoteId;
    n.title = m_editor->title().trimmed();
    if (n.title.isEmpty()) {
        n.title = QStringLiteral("Untitled Note");
        m_editor->setTitle(n.title);
    }
    n.body = m_editor->body();
    if (m_currentNoteId > 0) {
        n.body = m_sealedBlocks->processBodyForSave(m_currentNoteId, n.body, m_session.key());
    }
    n.isFavorite = m_editor->isFavorite();
    n.folderId = folderId;
    const QStringList tagParts = m_editor->tags().split(
        ',', Qt::SkipEmptyParts);
    for (const QString& t : tagParts) {
        n.tags.append(t.trimmed());
    }

    if (!m_notes->updateNote(n, m_session.key())) {
        m_editor->showSaveError(QStringLiteral("try again"));
        if (showErrorDialog) {
            DialogUtils::warning(
                this,
                QStringLiteral("Save Failed"),
                QStringLiteral("Could not save the note. Please try again."));
        }
        return false;
    }

    m_currentFolderId = folderId;
    const QDateTime savedAt = QDateTime::currentDateTimeUtc();
    m_editor->setSavedState(true, savedAt);
    m_savedLabel->setText(
        QStringLiteral("Last saved: %1").arg(TimeUtils::formatTimestamp(savedAt)));
    refreshSidebar();
    loadNotes();

    if (closeAfter) {
        closeNoteEditor();
    } else {
        m_noteList->selectNote(m_currentNoteId);
        updateStatusBar();
        updatePreview();
    }

    return true;
}

void MainWindow::closeNoteEditor() {
    m_currentNoteId = 0;
    m_currentFolderId = 0;
    m_editor->setIntegrityError(false);
    m_editor->setTitle(QString());
    m_editor->setBody(QString());
    m_editor->setTags(QString());
    m_editor->setFavorite(false);
    m_editor->setSavedState(true);
    m_editorSplitter->hide();
    m_emptyEditorLabel->show();
    m_noteList->clearSelection();
    m_savedLabel->setText(QStringLiteral("Last saved: —"));
    updateStatusBar();
}

void MainWindow::openNoteEditor() {
    m_emptyEditorLabel->hide();
    m_editorSplitter->show();
}

void MainWindow::onEditorChanged() {
    m_session.resetActivityTimer();
    updateStatusBar();
}

void MainWindow::onSearchChanged(const QString& query) {
    m_searchQuery = query;
    loadNotes();
}

void MainWindow::onSortChanged(NoteSortField field, bool descending) {
    m_sortField = field;
    m_sortDescending = descending;
    loadNotes();
}

void MainWindow::onLockVault() {
    if (m_lockingVault || !m_session.isUnlocked()) {
        return;
    }

    m_lockingVault = true;
    const bool credSaved = saveCurrentCredential(false);
    const bool noteSaved = saveCurrentNote(false, false, false);
    if (!credSaved || !noteSaved) {
        if (!DialogUtils::question(
                this,
                QStringLiteral("Lock Vault"),
                QStringLiteral("Some changes could not be saved. Lock anyway and discard unsaved edits?"))) {
            m_lockingVault = false;
            return;
        }
    }
    if (m_chronicle) {
        m_chronicle->appendEvent(
            QStringLiteral("vault_lock"),
            QStringLiteral("Vault locked"),
            m_session.key());
    }
    stopBridge();
    clearAllSensitiveState();
    m_chambers.reset();
    m_session.lock();
    m_lockingVault = false;
    emit vaultLocked();
}

void MainWindow::onViewModeChanged(int index) {
    switch (index) {
    case 0:
        m_editor->show();
        m_preview->show();
        break;
    case 1:
        m_editor->show();
        m_preview->hide();
        break;
    case 2:
        m_editor->hide();
        m_preview->show();
        updatePreview();
        break;
    }
}

void MainWindow::onFavoriteToggled(bool favorited) {
    Q_UNUSED(favorited);
    if (m_currentNoteId <= 0) {
        return;
    }
    saveCurrentNote(false, false, false);
}

void MainWindow::onNewFolder() {
    bool ok = false;
    const QString name = GrimInputDialog::getText(
        this, QStringLiteral("New Folder"),
        QStringLiteral("Folder name:"), &ok);
    if (ok && !name.isEmpty()) {
        m_notes->createFolder(name);
        refreshSidebar();
    }
}

void MainWindow::refreshSidebar() {
    m_sidebar->setFolders(m_notes->listFolders());
    m_sidebar->setTags(m_notes->listTags());
}

void MainWindow::showSettings() {
    saveCurrentCredential(false);
    refreshVaultHealth();
    m_settingsPanel->reloadFromStorage();
    m_rightStack->setCurrentWidget(m_settingsPanel);
}

void MainWindow::hideSettings() {
    applyPersistedSettings();
    m_rightStack->setCurrentWidget(m_contentStack);
    if (m_section == SidebarSection::Passwords) {
        showPasswordsMode();
    } else {
        showNotesMode();
    }
}

void MainWindow::onAccentPreviewChanged(const QString& hex) {
    applyAccent(hex, false);
}

void MainWindow::onAccentChanged(const QString& hex) {
    applyAccent(hex, true);
}

void MainWindow::applyPersistedSettings() {
    m_settingsPanel->reloadFromStorage();
    applyAccent(Theme::savedAccent(), false);
    m_editor->setLineNumbersVisible(AppSettings::lineNumbersEnabled());
    m_editor->setWordWrapEnabled(AppSettings::wordWrapEnabled());
    m_session.setAutoLockEnabled(AppSettings::autoLockEnabled());
    m_session.setAutoLockMinutes(AppSettings::autoLockMinutes());
    m_autolockLabel->setText(AppSettings::autoLockEnabled()
        ? QStringLiteral("Auto-lock: %1 min").arg(AppSettings::autoLockMinutes())
        : QStringLiteral("Auto-lock: off"));
    if (AppSettings::browserBridgeEnabled()) {
        startBridge();
    } else {
        stopBridge();
    }
    if (m_bridgeLabel) {
        m_bridgeLabel->setText(AppSettings::browserBridgeEnabled() && m_bridge
            ? QStringLiteral("Bridge: on")
            : QStringLiteral("Bridge: off"));
    }
}

void MainWindow::applyAccent(const QString& hex, bool persist) {
    m_accent = hex;
    if (persist) {
        Theme::saveAccent(hex);
    }
    m_editor->setAccentColor(QColor(hex));
    m_preview->setAccentColor(hex);
    if (QApplication* app = qApp) {
        Theme::apply(*app, hex);
    }
    updatePreview();
}

void MainWindow::onChangePassword(const QString& current, const QString& newPass) {
    QString err;
    if (!PasswordManager::isValidVaultPassword(newPass, &err)) {
        DialogUtils::warning(this, QStringLiteral("Password"), err);
        return;
    }

    const QByteArray currentKey = m_session.key();
    const auto vaultInfo = m_vault->loadVaultInfo();
    if (!vaultInfo) {
        DialogUtils::warning(this, QStringLiteral("Password"),
            QStringLiteral("Vault metadata is unavailable."));
        return;
    }

    auto testKey = CryptoManager::deriveKey(current, vaultInfo->salt, vaultInfo->kdfParams);
    if (!testKey || *testKey != currentKey) {
        DialogUtils::warning(
            this, QStringLiteral("Password"),
            QStringLiteral("Current password is incorrect."));
        if (testKey) {
            CryptoManager::secureZero(*testKey);
        }
        return;
    }
    CryptoManager::secureZero(*testKey);

    QByteArray newKey;
    if (!m_vault->changeMasterPassword(currentKey, newPass, newKey)) {
        DialogUtils::warning(
            this, QStringLiteral("Password"),
            QStringLiteral("Failed to change password. Vault unchanged."));
        return;
    }

    m_session.setKey(std::move(newKey));

    DialogUtils::information(
        this, QStringLiteral("Password"),
        QStringLiteral("Master password changed successfully."));
}

void MainWindow::onBackupVault(const QString& path) {
    bool ok = false;
    const QString password = GrimInputDialog::getPassword(
        this,
        QStringLiteral("Backup Vault"),
        QStringLiteral("Enter the master password to encrypt this backup:"),
        &ok);
    if (!ok || password.isEmpty()) {
        return;
    }

    if (PathSafety::isBlockedBackupTarget(path, m_db.path())) {
        DialogUtils::warning(
            this,
            QStringLiteral("Backup"),
            QStringLiteral("Cannot save a backup to the live vault file or restore working files."));
        return;
    }

    if (m_vault->exportEncryptedBackupV2(path, password)) {
        AppSettings::setLastEncryptedBackupTime(QDateTime::currentDateTimeUtc());
        if (m_chronicle) {
            m_chronicle->appendEvent(
                QStringLiteral("backup_created"),
                QStringLiteral("Encrypted backup saved"),
                m_session.key());
        }
        refreshVaultHealth();
        DialogUtils::information(this, QStringLiteral("Backup"),
            QStringLiteral("Encrypted backup saved (GRIMBKUP2)."));
    } else {
        DialogUtils::warning(
            this,
            QStringLiteral("Backup"),
            QStringLiteral("Backup failed. Check the password and destination path."));
    }
}

void MainWindow::onRestoreVault(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        DialogUtils::warning(this, QStringLiteral("Restore"),
            QStringLiteral("Could not open backup file."));
        return;
    }
    if (f.size() > SecurityLimits::kMaxBackupFileBytes) {
        DialogUtils::warning(this, QStringLiteral("Restore"),
            QStringLiteral("Backup file is too large."));
        return;
    }

    const QByteArray magic = f.read(9);
    f.close();

    QString error;
    RestoreResult restoreResult = RestoreResult::Failed;

    if (magic == QByteArray("GRIMBKUP2", 9)) {
        bool ok = false;
        const QString password = GrimInputDialog::getPassword(
            this,
            QStringLiteral("Restore Backup"),
            QStringLiteral("Enter the master password used when this backup was created:"),
            &ok);
        if (!ok || password.isEmpty()) {
            return;
        }

        m_db.close();
        restoreResult = VaultRepository::restoreFromBackup(path, password, m_db.path(), &error);
        if (!m_db.open(m_db.path())) {
            stopBridge();
            clearAllSensitiveState();
            m_session.lock();
            DialogUtils::critical(this, QStringLiteral("Restore"),
                QStringLiteral("Restored vault could not be reopened. Vault is locked."));
            emit vaultLocked();
            return;
        }
        m_vault = std::make_unique<VaultRepository>(m_db);
    } else if (magic == QByteArray("GRIMBKUP1", 9)) {
        if (!DialogUtils::question(
                this,
                QStringLiteral("Restore Legacy Backup"),
                QStringLiteral("This is a legacy session-bound backup. Restore only if it was created from this vault.\n\nContinue?"))) {
            return;
        }

        m_db.close();
        restoreResult = VaultRepository::restoreLegacyBackupInSession(
            path, m_session.key(), m_db.path(), &error);
        if (!m_db.open(m_db.path())) {
            stopBridge();
            clearAllSensitiveState();
            m_session.lock();
            DialogUtils::critical(this, QStringLiteral("Restore"),
                QStringLiteral("Restored vault could not be reopened. Vault is locked."));
            emit vaultLocked();
            return;
        }
        m_vault = std::make_unique<VaultRepository>(m_db);
    } else {
        DialogUtils::warning(this, QStringLiteral("Restore"),
            QStringLiteral("Invalid backup file."));
        return;
    }

    if (restoreResult == RestoreResult::Failed) {
        DialogUtils::warning(this, QStringLiteral("Restore"),
            error.isEmpty()
                ? QStringLiteral("Restore failed. Your original vault was not changed.")
                : error);
        return;
    }

    finalizeVaultRestore(
        restoreResult == RestoreResult::InstalledWithCleanupWarning,
        error);
}

void MainWindow::finalizeVaultRestore(bool cleanupWarning, const QString& warningDetail) {
    stopBridge();
    closeNoteEditor();
    clearAllSensitiveState();
    m_cachedNotes.clear();
    m_preview->setMarkdown(QString(), imageResolver());
    m_session.lock();

    QString message = QStringLiteral(
        "Vault restored from backup. Unlock with the password for this backup before editing.");
    if (cleanupWarning && !warningDetail.isEmpty()) {
        message += QStringLiteral("\n\n") + warningDetail;
    }

    DialogUtils::information(this, QStringLiteral("Restore"), message);
    emit vaultLocked();
}

void MainWindow::onImportMarkdown() {
    const QStringList files = GrimFileDialog::getOpenFileNames(
        this, QStringLiteral("Import Markdown"),
        QString(),
        QStringLiteral("Markdown (*.md)"));
    int count = 0;
    for (const QString& f : files) {
        if (m_notes->importMarkdownFile(f, m_session.key())) {
            ++count;
        }
    }
    loadNotes();
    refreshSidebar();
    DialogUtils::information(
        this, QStringLiteral("Import"),
        QStringLiteral("Imported %1 file(s).").arg(count));
}

void MainWindow::onExportNote() {
    if (m_currentNoteId <= 0) {
        DialogUtils::information(this, QStringLiteral("Export"),
            QStringLiteral("Select a note first."));
        return;
    }
    const QString path = GrimFileDialog::getSaveFileName(
        this, QStringLiteral("Export Note"),
        QStringLiteral("note.md"),
        QStringLiteral("Markdown (*.md)"));
    if (path.isEmpty()) {
        return;
    }

    const auto note = m_notes->getNote(m_currentNoteId, m_session.key());
    if (!note) {
        DialogUtils::warning(this, QStringLiteral("Export"),
            QStringLiteral("Could not read the note."));
        return;
    }

    const QFileInfo info(path);
    const QString imagesDir = info.absolutePath() + QLatin1Char('/')
        + info.completeBaseName() + QStringLiteral("_images");
    const QString body = m_attachments.rewriteBodyForExport(
        note->body, m_currentNoteId, imagesDir, m_session.key());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        DialogUtils::warning(this, QStringLiteral("Export"),
            QStringLiteral("Could not write export file."));
        return;
    }
    f.write(body.toUtf8());
    f.close();
}

void MainWindow::onExportAllMarkdown() {
    const QString dir = GrimFileDialog::getExistingDirectory(
        this, QStringLiteral("Export All Notes"));
    if (dir.isEmpty()) {
        return;
    }

    const ExportResult result = m_notes->exportAllMarkdown(dir, m_session.key());
    if (result.allOk()) {
        DialogUtils::information(
            this,
            QStringLiteral("Export"),
            QStringLiteral("Exported %1 note(s) as Markdown.").arg(result.ok));
        return;
    }

    DialogUtils::warning(
        this,
        QStringLiteral("Export"),
        QStringLiteral(
            "Exported %1 note(s). Failed: %2. Skipped integrity errors: %3. "
            "Filename collisions resolved: %4.")
            .arg(result.ok)
            .arg(result.failed)
            .arg(result.skippedIntegrity)
            .arg(result.collisions));
}

void MainWindow::onExportEncryptedArchive(const QString& path) {
    onBackupVault(path);
}

MarkdownRenderer::ImageUrlResolver MainWindow::imageResolver() const {
    const qint64 noteId = m_currentNoteId;
    const QByteArray key = m_session.key();
    return [this, noteId, key](const QString& url) -> QString {
        if (!AttachmentRepository::isGrimAttachmentUrl(url) || noteId <= 0 || key.isEmpty()) {
            return QString();
        }
        const QString attachmentId = AttachmentRepository::attachmentIdFromUrl(url);
        const QString cacheKey = QStringLiteral("%1:%2").arg(noteId).arg(attachmentId);
        if (m_attachmentPreviewCache.contains(cacheKey)) {
            return m_attachmentPreviewCache.value(cacheKey);
        }
        const auto data = m_attachments.loadAttachment(attachmentId, noteId, key);
        if (!data) {
            return QString();
        }
        const QString mime = m_attachments.mimeTypeForAttachment(attachmentId);
        const QString resolved = QStringLiteral("data:") + mime + QStringLiteral(";base64,") + data->toBase64();
        m_attachmentPreviewCache.insert(cacheKey, resolved);
        return resolved;
    };
}

void MainWindow::updatePreview() {
    m_preview->setMarkdown(m_editor->body(), imageResolver());
}

void MainWindow::onInsertImage() {
    if (m_currentNoteId <= 0) {
        DialogUtils::information(
            this,
            QStringLiteral("Insert Image"),
            QStringLiteral("Open or create a note before inserting an image."));
        return;
    }

    const QString path = GrimFileDialog::getOpenFileName(
        this,
        QStringLiteral("insert image"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp *.gif)"),
        QStringLiteral("metadata purged · encrypted in vault"));
    if (path.isEmpty()) {
        return;
    }

    SanitizedImage image;
    QString err;
    if (!ImageSanitizer::sanitizeFromFile(path, image, &err)) {
        DialogUtils::warning(this, QStringLiteral("Insert Image"), err);
        return;
    }

    const QString baseName = QFileInfo(path).completeBaseName();
    const QString attachmentId = m_attachments.storeAttachment(
        m_currentNoteId,
        image.data,
        image.mimeType,
        baseName,
        m_session.key());
    if (attachmentId.isEmpty()) {
        DialogUtils::warning(
            this,
            QStringLiteral("Insert Image"),
            QStringLiteral("Could not store image in the encrypted vault."));
        return;
    }

    const QString markdown = QStringLiteral("![%1](grim://attachment/%2)\n")
        .arg(baseName, attachmentId);
    m_editor->insertAtCursor(markdown);
    updatePreview();
    onEditorChanged();
}

void MainWindow::updateStatusBar() {
    m_wordLabel->setText(QStringLiteral("Words: %1").arg(m_editor->wordCount()));
    m_charLabel->setText(QStringLiteral("Chars: %1").arg(m_editor->charCount()));
}

void MainWindow::onVaultUnlocked() {
    m_credentials->migrateFillPolicies(m_session.key());
    m_chambers.unlockAll(m_session.key());
    if (m_chronicle) {
        m_chronicle->appendEvent(
            QStringLiteral("vault_unlock"),
            QStringLiteral("Vault unlocked"),
            m_session.key());
    }
    if (m_previewTimer) {
        m_previewTimer->start();
    }
    loadNotes();
    refreshSidebar();
    startBridge();
}

void MainWindow::clearAllSensitiveState() {
    closeNoteEditor();
    m_cachedNotes.clear();
    m_searchQuery.clear();
    m_attachmentPreviewCache.clear();
    if (m_preview) {
        m_preview->setMarkdown(QString());
    }
    if (m_previewTimer) {
        m_previewTimer->stop();
    }
    if (m_noteList) {
        m_noteList->clearSearch();
    }

    m_currentCredentialId = 0;
    m_credSearchQuery.clear();
    m_cachedCredentials.clear();
    m_credList->clearSelection();
    m_credEditor->clearFields();
}

void MainWindow::startBridge() {
    stopBridge();
    if (!AppSettings::browserBridgeEnabled() || !m_session.isUnlocked()) {
        if (m_bridgeLabel) {
            m_bridgeLabel->setText(QStringLiteral("Bridge: off"));
        }
        return;
    }

    m_bridge = std::make_unique<CredentialBridgeServer>(this);
    m_bridge->setRepository(m_credentials.get());
    m_bridge->setNoteRepository(m_notes.get());
    m_bridge->setFillCoordinator(m_fillCoordinator.get());
    m_bridge->setClipCoordinator(m_clipCoordinator.get());
    m_bridge->setUnlockedChecker([this]() { return m_session.isUnlocked(); });
    m_bridge->setSessionKeyProvider([this]() { return m_session.key(); });
    m_bridge->setBridgeEnabledChecker([]() { return AppSettings::browserBridgeEnabled(); });
    m_bridge->setClipEnabledChecker([]() { return AppSettings::webClipperEnabled(); });

    connect(
        m_fillCoordinator.get(),
        &BridgeFillCoordinator::confirmationRequested,
        this,
        [this](const QString& label, const QString& origin, QLocalSocket* socket) {
            QTimer::singleShot(0, this, [this, label, origin, socket]() {
                if (!m_bridge || !m_session.isUnlocked()) {
                    if (m_bridge) {
                        m_bridge->completeFillDecision(socket, false);
                    }
                    return;
                }
                const bool approved = DialogUtils::question(
                    this,
                    QStringLiteral("Browser Fill"),
                    QStringLiteral("Allow browser extension to fill \"%1\" on %2?")
                        .arg(label, origin));
                m_bridge->completeFillDecision(socket, approved && m_session.isUnlocked());
                if (m_chronicle) {
                    m_chronicle->appendEvent(
                        QStringLiteral("bridge_fill"),
                        approved
                            ? QStringLiteral("Fill approved for %1 on %2").arg(label, origin)
                            : QStringLiteral("Fill denied for %1 on %2").arg(label, origin),
                        m_session.key());
                }
            });
        },
        Qt::UniqueConnection);
    connect(
        m_clipCoordinator.get(),
        &BridgeClipCoordinator::confirmationRequested,
        this,
        [this](const QString& title, const QString& origin, const QString& preview, QLocalSocket* socket) {
            QTimer::singleShot(0, this, [this, title, origin, preview, socket]() {
                if (!m_bridge || !m_session.isUnlocked()) {
                    if (m_bridge) {
                        m_bridge->completeClipDecision(socket, false);
                    }
                    return;
                }
                const bool approved = DialogUtils::question(
                    this,
                    QStringLiteral("Web Clipper"),
                    QStringLiteral("Clip \"%1\" from %2?\n\nPreview:\n%3")
                        .arg(title, origin, preview));
                m_bridge->completeClipDecision(socket, approved && m_session.isUnlocked());
                if (m_chronicle && approved) {
                    m_chronicle->appendEvent(
                        QStringLiteral("web_clip"),
                        QStringLiteral("Clipped \"%1\" from %2").arg(title, origin),
                        m_session.key());
                }
            });
        },
        Qt::UniqueConnection);
    connect(m_bridge.get(), &CredentialBridgeServer::listenFailed, this, [this](const QString& reason) {
        if (m_bridgeLabel) {
            m_bridgeLabel->setText(QStringLiteral("Bridge: error"));
        }
        DialogUtils::warning(this, QStringLiteral("Browser Bridge"), reason);
    });

    if (!m_bridge->start()) {
        if (m_bridgeLabel) {
            m_bridgeLabel->setText(QStringLiteral("Bridge: error"));
        }
        m_bridge.reset();
        return;
    }

    if (m_bridgeLabel) {
        m_bridgeLabel->setText(QStringLiteral("Bridge: on"));
    }
}

void MainWindow::stopBridge() {
    if (!m_bridge) {
        if (m_bridgeLabel) {
            m_bridgeLabel->setText(QStringLiteral("Bridge: off"));
        }
        return;
    }
    if (m_fillCoordinator) {
        m_fillCoordinator->cancelAll();
    }
    if (m_clipCoordinator) {
        m_clipCoordinator->cancelAll();
    }
    m_bridge->cancelPendingRequests();
    m_bridge->stop();
    m_bridge.reset();
    disconnect(m_fillCoordinator.get(), &BridgeFillCoordinator::confirmationRequested, this, nullptr);
    disconnect(m_clipCoordinator.get(), &BridgeClipCoordinator::confirmationRequested, this, nullptr);
    if (m_bridgeLabel) {
        m_bridgeLabel->setText(QStringLiteral("Bridge: off"));
    }
}
