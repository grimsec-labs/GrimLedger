#pragma once

#include <QString>
#include <QVector>

struct RunbookStep {
    int index = 0;
    QString text;
    bool optional = false;
};

namespace RunbookParser {

QVector<RunbookStep> parseSteps(const QString& markdown);
bool isRunbookNote(const QString& markdown);

} // namespace RunbookParser
