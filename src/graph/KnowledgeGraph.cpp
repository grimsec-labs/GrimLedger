#include "graph/KnowledgeGraph.h"

#include <QRegularExpression>
#include <QSet>

namespace KnowledgeGraph {

KnowledgeGraphModel buildFromNotes(
    const QVector<Note>& notes,
    const QHash<qint64, QString>& bodiesById) {
    KnowledgeGraphModel model;
    QHash<QString, QString> titleById;
    QHash<QString, QSet<QString>> tagMembers;

    for (const Note& note : notes) {
        const QString nodeId = QStringLiteral("note:%1").arg(note.id);
        GraphNode node;
        node.id = nodeId;
        node.label = note.title;
        node.type = QStringLiteral("note");
        model.nodes.append(node);
        titleById[nodeId] = note.title;
        for (const QString& tag : note.tags) {
            const QString tagId = QStringLiteral("tag:%1").arg(tag);
            tagMembers[tagId].insert(nodeId);
        }
    }

    for (auto it = tagMembers.begin(); it != tagMembers.end(); ++it) {
        GraphNode tagNode;
        tagNode.id = it.key();
        tagNode.label = it.key().mid(4);
        tagNode.type = QStringLiteral("tag");
        model.nodes.append(tagNode);
        for (const QString& noteId : it.value()) {
            GraphEdge edge;
            edge.from = noteId;
            edge.to = it.key();
            edge.relation = QStringLiteral("tagged");
            model.edges.append(edge);
        }
    }

    static const QRegularExpression wikiLinkRe(QStringLiteral(R"(\[\[([^\]]+)\]\])"));
    static const QRegularExpression mdLinkRe(QStringLiteral(R"(\[([^\]]+)\]\(([^)]+)\))"));
    for (const Note& note : notes) {
        const QString body = bodiesById.value(note.id, note.body);
        const QString fromId = QStringLiteral("note:%1").arg(note.id);
        auto wikiIt = wikiLinkRe.globalMatch(body);
        while (wikiIt.hasNext()) {
            const QString targetTitle = wikiIt.next().captured(1).trimmed();
            for (const Note& target : notes) {
                if (target.title.compare(targetTitle, Qt::CaseInsensitive) == 0) {
                    GraphEdge edge;
                    edge.from = fromId;
                    edge.to = QStringLiteral("note:%1").arg(target.id);
                    edge.relation = QStringLiteral("links");
                    model.edges.append(edge);
                }
            }
        }
        auto mdIt = mdLinkRe.globalMatch(body);
        while (mdIt.hasNext()) {
            const auto match = mdIt.next();
            GraphEdge edge;
            edge.from = fromId;
            edge.to = QStringLiteral("url:%1").arg(match.captured(2));
            edge.relation = QStringLiteral("references");
            model.nodes.append({edge.to, match.captured(1), QStringLiteral("url")});
            model.edges.append(edge);
        }
    }

    return model;
}

QString toDot(const KnowledgeGraphModel& model) {
    QString dot = QStringLiteral("digraph G {\n");
    for (const GraphNode& node : model.nodes) {
        dot += QStringLiteral("  \"%1\" [label=\"%2\"];\n")
            .arg(node.id, node.label.toHtmlEscaped());
    }
    for (const GraphEdge& edge : model.edges) {
        dot += QStringLiteral("  \"%1\" -> \"%2\" [label=\"%3\"];\n")
            .arg(edge.from, edge.to, edge.relation);
    }
    dot += QStringLiteral("}\n");
    return dot;
}

} // namespace KnowledgeGraph
