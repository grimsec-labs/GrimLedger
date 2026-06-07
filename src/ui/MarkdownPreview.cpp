#include "ui/MarkdownPreview.h"
#include "markdown/MarkdownRenderer.h"

#include <QVBoxLayout>
#include <QTextBrowser>
#include <QUrl>

MarkdownPreview::MarkdownPreview(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_browser = new QTextBrowser(this);
    m_browser->setObjectName(QStringLiteral("MarkdownPreview"));
    m_browser->setOpenExternalLinks(false);
    connect(m_browser, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
        const QString scheme = url.scheme().toLower();
        if (scheme == QStringLiteral("https")
            || scheme == QStringLiteral("http")
            || scheme == QStringLiteral("mailto")) {
            m_browser->setSource(url);
        }
    });
    layout->addWidget(m_browser);
}

void MarkdownPreview::setMarkdown(
    const QString& markdown,
    const MarkdownRenderer::ImageUrlResolver& resolveImage) {
    m_browser->setHtml(MarkdownRenderer::renderToHtml(markdown, m_accent, resolveImage));
}

void MarkdownPreview::setAccentColor(const QString& hexColor) {
    m_accent = hexColor;
}
