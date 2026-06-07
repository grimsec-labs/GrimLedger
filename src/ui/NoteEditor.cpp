#include "ui/NoteEditor.h"
#include "markdown/SyntaxHighlighter.h"
#include "utils/TimeUtils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QFont>
#include <QTextCursor>
#include <QToolButton>
#include <QRegularExpression>

NoteEditor::NoteEditor(QWidget* parent)
    : QWidget(parent) {
    buildUi();
}

void NoteEditor::buildUi() {
    setObjectName(QStringLiteral("NoteEditor"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* header = new QHBoxLayout();
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setObjectName(QStringLiteral("TitleField"));
    m_titleEdit->setPlaceholderText(QStringLiteral("Note title"));
    connect(m_titleEdit, &QLineEdit::textChanged, this, &NoteEditor::onTextChanged);

    m_tagsEdit = new QLineEdit(this);
    m_tagsEdit->setObjectName(QStringLiteral("TagsField"));
    m_tagsEdit->setPlaceholderText(QStringLiteral("tags: comma separated"));
    connect(m_tagsEdit, &QLineEdit::textChanged, this, &NoteEditor::onTextChanged);

    m_favButton = new QPushButton(QStringLiteral("☆"), this);
    m_favButton->setObjectName(QStringLiteral("IconButton"));
    m_favButton->setFixedSize(32, 32);
    m_favButton->setToolTip(QStringLiteral("Toggle favorite"));
    connect(m_favButton, &QPushButton::clicked, this, [this]() {
        m_isFavorite = !m_isFavorite;
        m_favButton->setText(m_isFavorite ? QStringLiteral("★") : QStringLiteral("☆"));
        emit favoriteToggled(m_isFavorite);
    });

    m_saveButton = new QPushButton(QStringLiteral("Save && Close"), this);
    m_saveButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_saveButton->setToolTip(
        QStringLiteral("Pick a folder, save the note, and close the editor (Ctrl+S)"));
    connect(m_saveButton, &QPushButton::clicked, this, &NoteEditor::saveAndCloseRequested);

    header->addWidget(m_titleEdit, 3);
    header->addWidget(m_tagsEdit, 2);
    header->addWidget(m_saveButton);
    header->addWidget(m_favButton);

    m_toolbar = new QToolBar(this);
    m_toolbar->setObjectName(QStringLiteral("EditorToolbar"));
    const struct { const char* label; const char* before; const char* after; } actions[] = {
        {"H", "# ", ""},
        {"B", "**", "**"},
        {"I", "*", "*"},
        {"`", "`", "`"},
        {"```", "```\n", "\n```"},
        {">", "> ", ""},
        {"Link", "[", "](url)"},
        {"Img", "", ""},
        {"•", "- ", ""},
        {"1.", "1. ", ""},
        {"☐", "- [ ] ", ""},
    };
    for (const auto& a : actions) {
        auto* btn = new QToolButton(m_toolbar);
        btn->setText(QString::fromUtf8(a.label));
        if (QString::fromUtf8(a.label) == QStringLiteral("Img")) {
            btn->setToolTip(QStringLiteral("Insert image (metadata stripped)"));
            connect(btn, &QToolButton::clicked, this, &NoteEditor::imageInsertRequested);
        } else {
            btn->setProperty("md_before", QString::fromUtf8(a.before));
            btn->setProperty("md_after", QString::fromUtf8(a.after));
            connect(btn, &QToolButton::clicked, this, &NoteEditor::onToolbarAction);
        }
        m_toolbar->addWidget(btn);
    }

    m_editor = new QPlainTextEdit(this);
    m_editor->setObjectName(QStringLiteral("MarkdownEditor"));
    QFont mono(QStringLiteral("Consolas"));
    mono.setStyleHint(QFont::Monospace);
    m_editor->setFont(mono);
    m_highlighter = new MarkdownSyntaxHighlighter(m_editor->document());
    connect(m_editor, &QPlainTextEdit::textChanged, this, &NoteEditor::onTextChanged);

    m_saveLabel = new QLabel(QStringLiteral("Saved"), this);
    m_saveLabel->setObjectName(QStringLiteral("SaveStatus"));
    m_saveLabel->setToolTip(
        QStringLiteral("Autosaves quietly while you type. Use Save & Close to pick a folder and finish."));

    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setSingleShot(true);
    m_autosaveTimer->setInterval(1500);
    connect(m_autosaveTimer, &QTimer::timeout, this, &NoteEditor::onAutosaveTimer);

    layout->addLayout(header);
    layout->addWidget(m_toolbar);
    layout->addWidget(m_editor, 1);
    layout->addWidget(m_saveLabel);
}

void NoteEditor::setTitle(const QString& title) {
    m_titleEdit->blockSignals(true);
    m_titleEdit->setText(title);
    m_titleEdit->blockSignals(false);
}

void NoteEditor::setBody(const QString& body) {
    m_editor->blockSignals(true);
    m_editor->setPlainText(body);
    m_editor->blockSignals(false);
    m_highlighter->rehighlight();
    updateCounts();
}

void NoteEditor::setTags(const QString& tags) {
    m_tagsEdit->blockSignals(true);
    m_tagsEdit->setText(tags);
    m_tagsEdit->blockSignals(false);
}

void NoteEditor::setFavorite(bool favorited) {
    m_isFavorite = favorited;
    m_favButton->setText(favorited ? QStringLiteral("★") : QStringLiteral("☆"));
}

QString NoteEditor::title() const { return m_titleEdit->text(); }
QString NoteEditor::body() const { return m_editor->toPlainText(); }
QString NoteEditor::tags() const { return m_tagsEdit->text(); }

void NoteEditor::setViewMode(EditorViewMode mode) {
    Q_UNUSED(mode);
    // View mode handled by MainWindow splitter visibility
}

void NoteEditor::setAccentColor(const QColor& color) {
    m_highlighter->setAccentColor(color);
}

void NoteEditor::setLineNumbersVisible(bool visible) {
    Q_UNUSED(visible);
    // Line number gutter can be added via QPlainTextEdit subclass
}

void NoteEditor::setWordWrapEnabled(bool enabled) {
    if (enabled) {
        m_editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    } else {
        m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
}

void NoteEditor::setSavedState(bool saved, const QDateTime& savedAt) {
    m_saved = saved;
    if (savedAt.isValid()) {
        m_lastSaved = savedAt;
    }
    updateSaveIndicator();
}

int NoteEditor::wordCount() const {
    const QString t = m_editor->toPlainText();
    if (t.trimmed().isEmpty()) return 0;
    return t.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts).size();
}

int NoteEditor::charCount() const {
    return m_editor->toPlainText().size();
}

void NoteEditor::insertAtCursor(const QString& text) {
    QTextCursor cursor = m_editor->textCursor();
    cursor.insertText(text);
    m_editor->setTextCursor(cursor);
}

void NoteEditor::insertMarkdown(const QString& before, const QString& after) {
    QTextCursor cursor = m_editor->textCursor();
    const int start = cursor.selectionStart();
    const int end = cursor.selectionEnd();
    const QString selected = cursor.selectedText();

    cursor.beginEditBlock();
    cursor.insertText(before + selected + after);
    cursor.setPosition(start + before.size());
    cursor.setPosition(end + before.size(), QTextCursor::KeepAnchor);
    cursor.endEditBlock();
    m_editor->setTextCursor(cursor);
}

void NoteEditor::onTextChanged() {
    m_saved = false;
    updateSaveIndicator();
    updateCounts();
    m_autosaveTimer->start();
    emit contentChanged();
}

void NoteEditor::onAutosaveTimer() {
    emit saveRequested();
}

void NoteEditor::onToolbarAction() {
    const auto* btn = qobject_cast<QToolButton*>(sender());
    if (!btn) return;
    insertMarkdown(
        btn->property("md_before").toString(),
        btn->property("md_after").toString());
}

void NoteEditor::updateCounts() {
    // Counts displayed in MainWindow status bar
}

void NoteEditor::showSaveError(const QString& message) {
    m_saved = false;
    m_saveLabel->setText(QStringLiteral("✗ Save failed — %1").arg(message));
    m_saveLabel->setStyleSheet(QStringLiteral("color: #ff4444;"));
}

void NoteEditor::updateSaveIndicator() {
    if (m_saved) {
        m_saveLabel->setText(
            QStringLiteral("✓ Saved — %1").arg(TimeUtils::formatTimestamp(m_lastSaved)));
        m_saveLabel->setStyleSheet(QStringLiteral("color: #33cc66;"));
    } else {
        m_saveLabel->setText(QStringLiteral("● Unsaved changes"));
        m_saveLabel->setStyleSheet(QStringLiteral("color: #ff6600;"));
    }
}
