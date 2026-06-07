#include "utils/TimeUtils.h"

QString TimeUtils::formatRelative(const QDateTime& dt) {
    if (!dt.isValid()) {
        return QStringLiteral("—");
    }
    const qint64 secs = dt.secsTo(QDateTime::currentDateTime());
    if (secs < 60) return QStringLiteral("just now");
    if (secs < 3600) return QStringLiteral("%1 min ago").arg(secs / 60);
    if (secs < 86400) return QStringLiteral("%1 hr ago").arg(secs / 3600);
    return formatTimestamp(dt);
}

QString TimeUtils::formatTimestamp(const QDateTime& dt) {
    if (!dt.isValid()) {
        return QStringLiteral("—");
    }
    return dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

qint64 TimeUtils::toUnix(const QDateTime& dt) {
    return dt.toSecsSinceEpoch();
}

QDateTime TimeUtils::fromUnix(qint64 unix) {
    return QDateTime::fromSecsSinceEpoch(unix);
}
