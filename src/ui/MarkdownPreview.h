#pragma once

#include <QWidget>
#include "markdown/MarkdownRenderer.h"

class QTextBrowser;

class MarkdownPreview : public QWidget {
    Q_OBJECT

public:
    explicit MarkdownPreview(QWidget* parent = nullptr);

    void setMarkdown(
        const QString& markdown,
        const MarkdownRenderer::ImageUrlResolver& resolveImage = MarkdownRenderer::ImageUrlResolver());
    void setAccentColor(const QString& hexColor);

private:
    QTextBrowser* m_browser = nullptr;
    QString m_accent = QStringLiteral("#cc2200");
};
