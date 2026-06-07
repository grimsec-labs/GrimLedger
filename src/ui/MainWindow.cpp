#include "ui/MainWindow.h"
#include "ui/Sidebar.h"
#include "ui/NoteList.h"
#include "ui/NoteEditor.h"
#include "ui/MarkdownPreview.h"
#include "ui/SettingsWindow.h"
#include "ui/CustomTitleBar.h"
#include "ui/FramelessResize.h"
#include "storage/VaultRepository.h"
#include "storage/NoteRepository.h"
#include "search/SearchEngine.h"
#include "models/Tag.h"
#include "security/PasswordManager.h"
#include "utils/TimeUtils.h"
#include "utils/Theme.h"
#include "security/CryptoManager.h"
#include "security/VaultSession.h"

#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QFile>
#include <QTimer>
#include <QEvent>
#include <QAction>
#include <QKeySequence>
#include <QApplication>

MainWindow::MainWindow(Database& db, VaultSession& session, QWidget* parent)
    : QMainWindow(parent)
    , m_db(db)
    , m_session(session) {
    m_notes = std::make_unique<NoteRepository>(m_db);
    m_vault = std::make_unique<VaultRepository>(m_db);
    buildUi();
    loadNotes();
    refreshSidebar();
    selectInitialNote();

    connect(&m_session, &VaultSession::activityReset, this, [this]() {
        m_autolockLabel->setText(
            QStringLiteral("Auto-lock: %1 min").arg(m_session.autoLockMinutes()));
    });
    connect(&m_session, &VaultSession::locked, this, &MainWindow::onLockVault);

    QTimer* previewTimer = new QTimer(this);
    previewTimer->setInterval(400);
    connect(previewTimer, &QTimer::timeout, this, &MainWindow::updatePreview);
    previewTimer->start();
}

MainWindow::~MainWindow() = default;

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (FramelessResize::handleNativeEvent(this, eventType, message, result)) {
        return true;
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::buildUi() {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setWindowTitle(QStringLiteral("GrimLedger — Encrypted Markdown Vault"));
    setMinimumSize(1100, 700);
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
        if (QMessageBox::question(this, QStringLiteral("Delete"),
                QStringLiteral("Delete this note permanently?")) == QMessageBox::Yes) {
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
        const qint64 newId = m_notes->duplicateNote(id, m_session.key());
        if (newId > 0) {
            loadNotes();
            loadNote(newId);
        }
    });
    connect(m_noteList, &NoteList::searchChanged, this, &MainWindow::onSearchChanged);
    connect(m_noteList, &NoteList::sortChanged, this, &MainWindow::onSortChanged);

    m_mainPanel = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(m_mainPanel);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* viewRow = new QHBoxLayout();
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
    m_editor = new NoteEditor(this);
    m_preview = new MarkdownPreview(this);
    m_editorSplitter->addWidget(m_editor);
    m_editorSplitter->addWidget(m_preview);
    m_editorSplitter->setStretchFactor(0, 1);
    m_editorSplitter->setStretchFactor(1, 1);

    connect(m_editor, &NoteEditor::contentChanged, this, &MainWindow::onEditorChanged);
    connect(m_editor, &NoteEditor::saveRequested, this, &MainWindow::onSaveNote);
    connect(m_editor, &NoteEditor::favoriteToggled, this, &MainWindow::onFavoriteToggled);

    mainLayout->addLayout(viewRow);
    mainLayout->addWidget(m_editorSplitter, 1);

    m_settingsPanel = new SettingsWindow(this);
    m_settingsPanel->hide();
    connect(m_settingsPanel, &SettingsWindow::backRequested, this, &MainWindow::hideSettings);
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
    connect(m_settingsPanel, &SettingsWindow::changePasswordRequested,
            this, &MainWindow::onChangePassword);
    connect(m_settingsPanel, &SettingsWindow::backupVaultRequested, this, &MainWindow::onBackupVault);
    connect(m_settingsPanel, &SettingsWindow::restoreVaultRequested, this, &MainWindow::onRestoreVault);
    connect(m_settingsPanel, &SettingsWindow::importMarkdownRequested, this, &MainWindow::onImportMarkdown);
    connect(m_settingsPanel, &SettingsWindow::exportNoteRequested, this, &MainWindow::onExportNote);
    connect(m_settingsPanel, &SettingsWindow::exportAllMarkdownRequested, this, &MainWindow::onExportAllMarkdown);
    connect(m_settingsPanel, &SettingsWindow::exportEncryptedArchiveRequested,
            this, &MainWindow::onExportEncryptedArchive);

    auto* rightStack = new QVBoxLayout();
    rightStack->setContentsMargins(0, 0, 0, 0);
    rightStack->addWidget(m_mainPanel);
    rightStack->addWidget(m_settingsPanel);

    auto* rightWidget = new QWidget(this);
    rightWidget->setLayout(rightStack);

    rootLayout->addWidget(m_sidebar);
    rootLayout->addWidget(m_noteList);
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
        QStringLiteral("Auto-lock: %1 min").arg(m_session.autoLockMinutes()), this);

    auto* lockBtn = new QPushButton(QStringLiteral("Lock"), this);
    lockBtn->setObjectName(QStringLiteral("SmallButton"));
    connect(lockBtn, &QPushButton::clicked, this, &MainWindow::onLockVault);

    m_status->addWidget(m_wordLabel);
    m_status->addWidget(new QLabel(QStringLiteral(" | "), this));
    m_status->addWidget(m_charLabel);
    m_status->addWidget(new QLabel(QStringLiteral(" | "), this));
    m_status->addWidget(m_savedLabel);
    m_status->addPermanentWidget(m_encryptLabel);
    m_status->addPermanentWidget(m_autolockLabel);
    m_status->addPermanentWidget(lockBtn);

    auto* saveAction = new QAction(this);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveNote);
    addAction(saveAction);

    applyAccent(m_accent);
    installEventFilter(this);
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
                bodies.insert(n.id, full->body);
            }
        }
        const auto matches = SearchEngine::search(m_cachedNotes, m_searchQuery, true, bodies);
        QVector<Note> filtered;
        for (const SearchMatch& m : matches) {
            Note copy = m.note;
            if (bodies.contains(copy.id)) {
                copy.body = bodies[copy.id];
            }
            filtered.append(copy);
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
    m_section = section;
    m_filterId = id;
    loadNotes();
}

void MainWindow::onNoteSelected(qint64 id) {
    if (m_currentNoteId > 0 && m_currentNoteId != id) {
        onSaveNote();
    }
    loadNote(id);
}

void MainWindow::onNewNote() {
    Note n;
    n.title = QStringLiteral("Untitled Note");
    n.body = QStringLiteral("# New Note\n\nBegin writing...");
    const qint64 id = m_notes->createNote(n, m_session.key());
    if (id > 0) {
        loadNotes();
        loadNote(id);
        m_noteList->selectNote(id);
    }
}

void MainWindow::onSaveNote() {
    if (m_currentNoteId <= 0) {
        return;
    }

    Note n;
    n.id = m_currentNoteId;
    n.title = m_editor->title().trimmed();
    if (n.title.isEmpty()) {
        n.title = QStringLiteral("Untitled Note");
        m_editor->setTitle(n.title);
    }
    n.body = m_editor->body();
    n.isFavorite = m_editor->isFavorite();
    const QStringList tagParts = m_editor->tags().split(
        ',', Qt::SkipEmptyParts);
    for (const QString& t : tagParts) {
        n.tags.append(t.trimmed());
    }

    if (m_notes->updateNote(n, m_session.key())) {
        const QDateTime savedAt = QDateTime::currentDateTimeUtc();
        m_editor->setSavedState(true, savedAt);
        m_savedLabel->setText(
            QStringLiteral("Last saved: %1").arg(TimeUtils::formatTimestamp(savedAt)));
        refreshSidebar();
        loadNotes();
        m_noteList->selectNote(m_currentNoteId);
        updateStatusBar();
        updatePreview();
    }
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
    onSaveNote();
    m_session.lock();
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
    if (m_currentNoteId <= 0) return;
    const auto note = m_notes->getNote(m_currentNoteId, m_session.key());
    if (!note) return;
    Note n = *note;
    n.isFavorite = favorited;
    m_notes->updateNote(n, m_session.key());
    loadNotes();
}

void MainWindow::onNewFolder() {
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, QStringLiteral("New Folder"),
        QStringLiteral("Folder name:"),
        QLineEdit::Normal, QString(), &ok);
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
    m_mainPanel->hide();
    m_settingsPanel->show();
}

void MainWindow::hideSettings() {
    m_settingsPanel->hide();
    m_mainPanel->show();
}

void MainWindow::onAccentChanged(const QString& hex) {
    applyAccent(hex);
}

void MainWindow::applyAccent(const QString& hex) {
    m_accent = hex;
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
        QMessageBox::warning(this, QStringLiteral("Password"), err);
        return;
    }

    const QByteArray currentKey = m_session.key();
    auto testKey = CryptoManager::deriveKey(
        current,
        m_vault->loadVaultInfo()->salt,
        m_vault->loadVaultInfo()->kdfParams);

    if (!testKey || *testKey != currentKey) {
        QMessageBox::warning(
            this, QStringLiteral("Password"),
            QStringLiteral("Current password is incorrect."));
        if (testKey) CryptoManager::secureZero(*testKey);
        return;
    }
    CryptoManager::secureZero(*testKey);

    if (!m_vault->changeMasterPassword(currentKey, newPass)) {
        QMessageBox::warning(
            this, QStringLiteral("Password"),
            QStringLiteral("Failed to change password. Vault unchanged."));
        return;
    }

    auto newKey = CryptoManager::deriveKey(
        newPass,
        m_vault->loadVaultInfo()->salt,
        m_vault->loadVaultInfo()->kdfParams);
    if (newKey) {
        m_session.setKey(std::move(*newKey));
    }

    QMessageBox::information(
        this, QStringLiteral("Password"),
        QStringLiteral("Master password changed successfully."));
}

void MainWindow::onBackupVault(const QString& path) {
    if (m_vault->exportEncryptedBackup(m_session.key(), path)) {
        QMessageBox::information(this, QStringLiteral("Backup"),
            QStringLiteral("Encrypted backup saved."));
    } else {
        QMessageBox::warning(this, QStringLiteral("Backup"),
            QStringLiteral("Backup failed."));
    }
}

void MainWindow::onRestoreVault(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("Restore"),
            QStringLiteral("Could not open backup file."));
        return;
    }

    const QByteArray magic = f.read(9);
    if (magic != QByteArray("GRIMBKUP1", 9)) {
        QMessageBox::warning(this, QStringLiteral("Restore"),
            QStringLiteral("Invalid backup file."));
        return;
    }

    const auto decrypted = CryptoManager::decrypt(f.readAll(), m_session.key());
    if (!decrypted) {
        QMessageBox::warning(this, QStringLiteral("Restore"),
            QStringLiteral("Could not decrypt backup. Wrong vault or corrupted file."));
        return;
    }

    m_db.close();
    QFile vaultFile(m_db.path());
    vaultFile.remove();
    if (!vaultFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, QStringLiteral("Restore"),
            QStringLiteral("Could not write vault file."));
        return;
    }
    QByteArray plain = *decrypted;
    vaultFile.write(plain);
    vaultFile.close();
    CryptoManager::secureZero(plain);

    if (!m_db.open(m_db.path())) {
        QMessageBox::critical(this, QStringLiteral("Restore"),
            QStringLiteral("Restored file could not be opened."));
        return;
    }

    loadNotes();
    refreshSidebar();
    QMessageBox::information(this, QStringLiteral("Restore"),
        QStringLiteral("Vault restored from backup."));
}

void MainWindow::onImportMarkdown() {
    const QStringList files = QFileDialog::getOpenFileNames(
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
    QMessageBox::information(
        this, QStringLiteral("Import"),
        QStringLiteral("Imported %1 file(s).").arg(count));
}

void MainWindow::onExportNote() {
    if (m_currentNoteId <= 0) {
        QMessageBox::information(this, QStringLiteral("Export"),
            QStringLiteral("Select a note first."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export Note"),
        QStringLiteral("note.md"),
        QStringLiteral("Markdown (*.md)"));
    if (!path.isEmpty()) {
        m_notes->exportMarkdownFile(m_currentNoteId, path, m_session.key());
    }
}

void MainWindow::onExportAllMarkdown() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Export All Notes"));
    if (!dir.isEmpty()) {
        m_notes->exportAllMarkdown(dir, m_session.key());
        QMessageBox::information(this, QStringLiteral("Export"),
            QStringLiteral("Notes exported as Markdown."));
    }
}

void MainWindow::onExportEncryptedArchive(const QString& path) {
    onBackupVault(path);
}

void MainWindow::updatePreview() {
    m_preview->setMarkdown(m_editor->body());
}

void MainWindow::updateStatusBar() {
    m_wordLabel->setText(QStringLiteral("Words: %1").arg(m_editor->wordCount()));
    m_charLabel->setText(QStringLiteral("Chars: %1").arg(m_editor->charCount()));
}
