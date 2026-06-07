#include "runbook/RunbookParser.h"

#include <QRegularExpression>

namespace RunbookParser {

QVector<RunbookStep> parseSteps(const QString& markdown) {
    QVector<RunbookStep> steps;
    static const QRegularExpression taskRe(
        QStringLiteral(R"(^\s*-\s+\[([ xX])\]\s+(.*)$)"));
    static const QRegularExpression numberedRe(
        QStringLiteral(R"(^\s*\d+\.\s+(.*)$)"));

    const QStringList lines = markdown.split('\n');
    int index = 0;
    for (const QString& line : lines) {
        auto task = taskRe.match(line);
        if (task.hasMatch()) {
            RunbookStep step;
            step.index = ++index;
            step.text = task.captured(2).trimmed();
            step.optional = step.text.startsWith(QStringLiteral("(optional)"), Qt::CaseInsensitive);
            steps.append(step);
            continue;
        }
        auto numbered = numberedRe.match(line);
        if (numbered.hasMatch()) {
            RunbookStep step;
            step.index = ++index;
            step.text = numbered.captured(1).trimmed();
            steps.append(step);
        }
    }
    return steps;
}

bool isRunbookNote(const QString& markdown) {
    return parseSteps(markdown).size() >= 2;
}

} // namespace RunbookParser
