#pragma once

#include <QString>
#include <functional>

class MarkdownRenderer {
public:
    using ImageUrlResolver = std::function<QString(const QString& url)>;

    static QString renderToHtml(
        const QString& markdown,
        const QString& accentColor = "#cc2200",
        const ImageUrlResolver& resolveImage = ImageUrlResolver());
    static QString escapeHtml(const QString& text);
};
