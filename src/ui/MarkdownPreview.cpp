#include "ui/MarkdownPreview.h"
#include "markdown/MarkdownRenderer.h"

#include <QVBoxLayout>
#include <QTextBrowser>

MarkdownPreview::MarkdownPreview(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_browser = new QTextBrowser(this);
    m_browser->setObjectName(QStringLiteral("MarkdownPreview"));
    m_browser->setOpenExternalLinks(true);
    layout->addWidget(m_browser);
}

void MarkdownPreview::setMarkdown(const QString& markdown) {
    m_browser->setHtml(MarkdownRenderer::renderToHtml(markdown, m_accent));
}

void MarkdownPreview::setAccentColor(const QString& hexColor) {
    m_accent = hexColor;
}
