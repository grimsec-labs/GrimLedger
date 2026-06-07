#pragma once

#include <QWidget>
#include <QDateTime>

class QTimer;

class QPlainTextEdit;
class QLineEdit;
class QToolBar;
class QPushButton;
class QLabel;
class MarkdownSyntaxHighlighter;

enum class EditorViewMode { Split, EditorOnly, PreviewOnly };

class NoteEditor : public QWidget {
    Q_OBJECT

public:
    explicit NoteEditor(QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setBody(const QString& body);
    void setTags(const QString& tags);
    void setFavorite(bool favorited);
    QString title() const;
    QString body() const;
    QString tags() const;
    bool isFavorite() const { return m_isFavorite; }

    void setViewMode(EditorViewMode mode);
    void setAccentColor(const QColor& color);
    void setLineNumbersVisible(bool visible);
    void setWordWrapEnabled(bool enabled);

    void setSavedState(bool saved, const QDateTime& savedAt = QDateTime());
    void setIntegrityError(bool errored);
    bool integrityError() const;
    void showSaveError(const QString& message);
    int wordCount() const;
    int charCount() const;

signals:
    void contentChanged();
    void favoriteToggled(bool favorited);
    void saveRequested();
    void saveAndCloseRequested();
    void imageInsertRequested();

public slots:
    void insertMarkdown(const QString& before, const QString& after = QString());
    void insertAtCursor(const QString& text);

private slots:
    void onTextChanged();
    void onAutosaveTimer();
    void onToolbarAction();

private:
    void buildUi();
    void updateCounts();
    void updateSaveIndicator();

    QLineEdit* m_titleEdit = nullptr;
    QLineEdit* m_tagsEdit = nullptr;
    QPlainTextEdit* m_editor = nullptr;
    QToolBar* m_toolbar = nullptr;
    QLabel* m_saveLabel = nullptr;
    QPushButton* m_favButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    MarkdownSyntaxHighlighter* m_highlighter = nullptr;
    QTimer* m_autosaveTimer = nullptr;
    bool m_saved = true;
    QDateTime m_lastSaved;
    bool m_isFavorite = false;
    bool m_integrityError = false;
};
