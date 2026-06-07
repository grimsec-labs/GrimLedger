#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QString>
#include <QHash>

// Language-aware syntax highlighter for fenced code blocks inside Markdown.
// Uses regex-based rules modeled after common highlighter grammars.
class SyntaxHighlighterEngine {
public:
    struct TokenRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    static QStringList supportedLanguages();
    static QString normalizeLanguage(const QString& lang);
    static QVector<TokenRule> rulesForLanguage(const QString& lang);
    static QString highlightToHtml(const QString& code, const QString& lang);
};

class MarkdownSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit MarkdownSyntaxHighlighter(QTextDocument* parent = nullptr);

    void setAccentColor(const QColor& color);

protected:
    void highlightBlock(const QString& text) override;

private:
    void highlightMarkdown(const QString& text);
    void highlightCodeBlock(const QString& text, const QString& lang);

    QTextCharFormat m_headingFormat;
    QTextCharFormat m_boldFormat;
    QTextCharFormat m_italicFormat;
    QTextCharFormat m_inlineCodeFormat;
    QTextCharFormat m_linkFormat;
    QTextCharFormat m_blockquoteFormat;
    QTextCharFormat m_listFormat;
    QTextCharFormat m_fenceFormat;
    QColor m_accent;
};
