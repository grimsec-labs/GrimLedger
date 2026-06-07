#pragma once

#include <QString>

class MarkdownRenderer {
public:
    static QString renderToHtml(const QString& markdown, const QString& accentColor = "#cc2200");
    static QString escapeHtml(const QString& text);
};
