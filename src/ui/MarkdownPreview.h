#pragma once

#include <QWidget>

class QTextBrowser;

class MarkdownPreview : public QWidget {
    Q_OBJECT

public:
    explicit MarkdownPreview(QWidget* parent = nullptr);

    void setMarkdown(const QString& markdown);
    void setAccentColor(const QString& hexColor);

private:
    QTextBrowser* m_browser = nullptr;
    QString m_accent = QStringLiteral("#cc2200");
};
