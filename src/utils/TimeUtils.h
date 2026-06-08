#pragma once

#include <QDateTime>
#include <QString>

class TimeUtils {
public:
    static QString formatRelative(const QDateTime& dt);
    static QString formatTimestamp(const QDateTime& dt);
    static qint64 toUnix(const QDateTime& dt);
    static QDateTime fromUnix(qint64 epochSeconds);
};
