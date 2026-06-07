#pragma once

#include "models/Note.h"

#include <QHash>
#include <QString>
#include <QVector>

struct GraphNode {
    QString id;
    QString label;
    QString type;
};

struct GraphEdge {
    QString from;
    QString to;
    QString relation;
};

struct KnowledgeGraphModel {
    QVector<GraphNode> nodes;
    QVector<GraphEdge> edges;
};

namespace KnowledgeGraph {

KnowledgeGraphModel buildFromNotes(
    const QVector<Note>& notes,
    const QHash<qint64, QString>& bodiesById);
QString toDot(const KnowledgeGraphModel& model);

} // namespace KnowledgeGraph
