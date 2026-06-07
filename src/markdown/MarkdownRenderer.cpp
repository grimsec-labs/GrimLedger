#include "markdown/MarkdownRenderer.h"
#include "markdown/SyntaxHighlighter.h"

#include <QRegularExpression>

namespace {

QString inlineFormat(const QString& line) {
    QString output;
    int pos = 0;
    const int n = line.size();

    while (pos < n) {
        if (line.mid(pos).startsWith(QStringLiteral("**"))) {
            const int end = line.indexOf(QStringLiteral("**"), pos + 2);
            if (end != -1) {
                output += QStringLiteral("<strong>")
                    + MarkdownRenderer::escapeHtml(line.mid(pos + 2, end - pos - 2))
                    + QStringLiteral("</strong>");
                pos = end + 2;
                continue;
            }
        }
        if (line[pos] == QLatin1Char('*')
            && (pos + 1 >= n || line[pos + 1] != QLatin1Char('*'))) {
            const int end = line.indexOf(QLatin1Char('*'), pos + 1);
            if (end != -1) {
                output += QStringLiteral("<em>")
                    + MarkdownRenderer::escapeHtml(line.mid(pos + 1, end - pos - 1))
                    + QStringLiteral("</em>");
                pos = end + 1;
                continue;
            }
        }
        if (line[pos] == QLatin1Char('`')) {
            const int end = line.indexOf(QLatin1Char('`'), pos + 1);
            if (end != -1) {
                output += QStringLiteral("<code>")
                    + MarkdownRenderer::escapeHtml(line.mid(pos + 1, end - pos - 1))
                    + QStringLiteral("</code>");
                pos = end + 1;
                continue;
            }
        }
        if (line[pos] == QLatin1Char('[')) {
            static const QRegularExpression linkRe(
                QStringLiteral(R"(\[([^\]]+)\]\(([^)]+)\))"));
            const auto m = linkRe.match(line, pos);
            if (m.hasMatch() && m.capturedStart() == pos) {
                output += QStringLiteral(R"(<a href=")")
                    + MarkdownRenderer::escapeHtml(m.captured(2))
                    + QStringLiteral(R"(">)")
                    + MarkdownRenderer::escapeHtml(m.captured(1))
                    + QStringLiteral("</a>");
                pos = m.capturedEnd();
                continue;
            }
        }

        int next = pos + 1;
        while (next < n
               && line[next] != QLatin1Char('*')
               && line[next] != QLatin1Char('`')
               && line[next] != QLatin1Char('[')) {
            ++next;
        }
        output += MarkdownRenderer::escapeHtml(line.mid(pos, next - pos));
        pos = next;
    }
    return output;
}

} // namespace

QString MarkdownRenderer::escapeHtml(const QString& text) {
    return text.toHtmlEscaped();
}

QString MarkdownRenderer::renderToHtml(const QString& markdown, const QString& accentColor) {
    QString html;
    html += QStringLiteral(
        "<html><head><style>"
        "body{background:#0a0a0c;color:#d8d8dc;font-family:'Segoe UI',sans-serif;"
        "padding:16px;line-height:1.6;}"
        "h1,h2,h3,h4{color:%1;border-bottom:1px solid #331111;padding-bottom:4px;}"
        "code{background:#151518;color:#66ff99;padding:2px 5px;border-radius:3px;"
        "font-family:Consolas,monospace;font-size:0.9em;}"
        "pre{background:#0f0f12;border:1px solid #331111;border-left:3px solid %1;"
        "padding:12px;overflow-x:auto;border-radius:4px;}"
        "pre code{background:transparent;padding:0;}"
        "blockquote{border-left:3px solid %1;color:#998877;margin:8px 0;padding:4px 12px;}"
        "a{color:#44ccff;}"
        "ul,ol{margin-left:20px;}"
        "table{border-collapse:collapse;width:100%;}"
        "th,td{border:1px solid #333;padding:6px;}"
        "th{background:#1a1a1e;color:%1;}"
        ".search-hit{background:#442200;color:#ffcc00;}"
        "hr{border:none;border-top:1px solid #331111;margin:16px 0;}"
        "</style></head><body>").arg(accentColor);

    const QStringList lines = markdown.split('\n');
    bool inCode = false;
    QString codeLang;
    QString codeBuffer;
    bool inUl = false;
    bool inOl = false;

    auto closeLists = [&]() {
        if (inUl) { html += QStringLiteral("</ul>"); inUl = false; }
        if (inOl) { html += QStringLiteral("</ol>"); inOl = false; }
    };

    for (const QString& line : lines) {
        static const QRegularExpression fenceRe(QStringLiteral(R"(^```([\w\+\#\.-]*)\s*$)"));
        const auto fence = fenceRe.match(line);
        if (fence.hasMatch()) {
            if (!inCode) {
                closeLists();
                inCode = true;
                codeLang = fence.captured(1);
                codeBuffer.clear();
            } else {
                const QString highlighted = SyntaxHighlighterEngine::highlightToHtml(
                    codeBuffer, codeLang);
                html += QStringLiteral("<pre><code class='language-")
                    + escapeHtml(codeLang)
                    + QStringLiteral("'>")
                    + highlighted
                    + QStringLiteral("</code></pre>");
                inCode = false;
                codeLang.clear();
                codeBuffer.clear();
            }
            continue;
        }

        if (inCode) {
            codeBuffer += line + '\n';
            continue;
        }

        if (line.trimmed().isEmpty()) {
            closeLists();
            html += QStringLiteral("<br/>");
            continue;
        }

        if (line.startsWith(QStringLiteral("### "))) {
            closeLists();
            html += QStringLiteral("<h3>") + escapeHtml(line.mid(4)) + QStringLiteral("</h3>");
            continue;
        }
        if (line.startsWith(QStringLiteral("## "))) {
            closeLists();
            html += QStringLiteral("<h2>") + escapeHtml(line.mid(3)) + QStringLiteral("</h2>");
            continue;
        }
        if (line.startsWith(QStringLiteral("# "))) {
            closeLists();
            html += QStringLiteral("<h1>") + escapeHtml(line.mid(2)) + QStringLiteral("</h1>");
            continue;
        }

        if (line.trimmed() == QStringLiteral("---") || line.trimmed() == QStringLiteral("***")) {
            closeLists();
            html += QStringLiteral("<hr/>");
            continue;
        }

        if (line.startsWith(QStringLiteral("> "))) {
            closeLists();
            html += QStringLiteral("<blockquote>") + escapeHtml(line.mid(2)) + QStringLiteral("</blockquote>");
            continue;
        }

        static const QRegularExpression taskRe(QStringLiteral(R"(^\s*-\s+\[([ xX])\]\s+(.*))"));
        const auto task = taskRe.match(line);
        if (task.hasMatch()) {
            if (!inUl) { closeLists(); html += QStringLiteral("<ul>"); inUl = true; }
            const bool checked = task.captured(1).toLower() == QStringLiteral("x");
            html += QStringLiteral("<li><input type='checkbox' disabled ")
                + (checked ? QStringLiteral("checked ") : QString())
                + QStringLiteral("/> ")
                + inlineFormat(task.captured(2))
                + QStringLiteral("</li>");
            continue;
        }

        static const QRegularExpression ulRe(QStringLiteral(R"(^\s*[-*+]\s+(.*))"));
        const auto ul = ulRe.match(line);
        if (ul.hasMatch()) {
            if (!inUl) { closeLists(); html += QStringLiteral("<ul>"); inUl = true; }
            html += QStringLiteral("<li>") + inlineFormat(ul.captured(1)) + QStringLiteral("</li>");
            continue;
        }

        static const QRegularExpression olRe(QStringLiteral(R"(^\s*\d+\.\s+(.*))"));
        const auto ol = olRe.match(line);
        if (ol.hasMatch()) {
            if (!inOl) { closeLists(); html += QStringLiteral("<ol>"); inOl = true; }
            html += QStringLiteral("<li>") + inlineFormat(ol.captured(1)) + QStringLiteral("</li>");
            continue;
        }

        closeLists();
        html += QStringLiteral("<p>") + inlineFormat(line) + QStringLiteral("</p>");
    }

    if (inCode && !codeBuffer.isEmpty()) {
        const QString highlighted = SyntaxHighlighterEngine::highlightToHtml(codeBuffer, codeLang);
        html += QStringLiteral("<pre><code>") + highlighted + QStringLiteral("</code></pre>");
    }

    closeLists();
    html += QStringLiteral("</body></html>");
    return html;
}
