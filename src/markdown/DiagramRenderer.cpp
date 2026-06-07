#include "markdown/DiagramRenderer.h"

#include <QHash>
#include <QPair>
#include <QRegularExpression>
#include <QSet>

namespace DiagramRenderer {

bool isDiagramLanguage(const QString& lang) {
    const QString lower = lang.trimmed().toLower();
    return lower == QStringLiteral("mermaid")
        || lower == QStringLiteral("flowchart")
        || lower == QStringLiteral("graph");
}

QString renderMermaidSubset(const QString& source, const QString& accentColor) {
    QStringList lines = source.split('\n', Qt::SkipEmptyParts);
    QSet<QString> nodes;
    QVector<QPair<QString, QString>> edges;

    static const QRegularExpression edgeRe(
        QStringLiteral(R"((\w+)\s*-->\s*(\w+))"));
    static const QRegularExpression flowHeader(
        QStringLiteral(R"(^(flowchart|graph)\s+(LR|TB|RL|BT))"), QRegularExpression::CaseInsensitiveOption);

    for (QString line : lines) {
        line = line.trimmed();
        if (flowHeader.match(line).hasMatch()) {
            continue;
        }
        const auto match = edgeRe.match(line);
        if (match.hasMatch()) {
            const QString from = match.captured(1);
            const QString to = match.captured(2);
            nodes.insert(from);
            nodes.insert(to);
            edges.append({from, to});
        }
    }

    if (nodes.isEmpty()) {
        return QStringLiteral("<pre class='diagram-fallback'>")
            + source.toHtmlEscaped()
            + QStringLiteral("</pre>");
    }

    QString svg = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 640 240' "
        "style='max-width:100%;background:#0f0f12;border:1px solid #331111;'>");
    int x = 40;
    QHash<QString, int> xPos;
    for (const QString& node : nodes) {
        xPos[node] = x;
        svg += QStringLiteral("<rect x='%1' y='90' width='100' height='40' rx='6' fill='#151518' stroke='%2'/>")
            .arg(x).arg(accentColor);
        svg += QStringLiteral("<text x='%1' y='115' fill='#d8d8dc' font-size='12' text-anchor='middle'>%2</text>")
            .arg(x + 50).arg(node.toHtmlEscaped());
        x += 140;
    }
    for (const auto& edge : edges) {
        const int x1 = xPos.value(edge.first) + 100;
        const int x2 = xPos.value(edge.second);
        svg += QStringLiteral("<line x1='%1' y1='110' x2='%2' y2='110' stroke='%3' marker-end='url(#arrow)'/>")
            .arg(x1).arg(x2).arg(accentColor);
    }
    svg += QStringLiteral("<defs><marker id='arrow' markerWidth='8' markerHeight='8' refX='6' refY='3' orient='auto'>"
        "<path d='M0,0 L6,3 L0,6 z' fill='%1'/></marker></defs>").arg(accentColor);
    svg += QStringLiteral("</svg>");
    return svg;
}

} // namespace DiagramRenderer
