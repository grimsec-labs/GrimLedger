#pragma once

#include <QFileInfo>
#include <QString>

namespace PathSafety {

inline bool samePath(const QString& a, const QString& b) {
    if (a.isEmpty() || b.isEmpty()) {
        return false;
    }
    const QFileInfo fa(a);
    const QFileInfo fb(b);
    const QString ca = fa.canonicalFilePath();
    const QString cb = fb.canonicalFilePath();
    if (!ca.isEmpty() && !cb.isEmpty()) {
        return QString::compare(ca, cb, Qt::CaseInsensitive) == 0;
    }
    return QString::compare(
               fa.absoluteFilePath(), fb.absoluteFilePath(), Qt::CaseInsensitive)
        == 0;
}

inline bool isBlockedBackupTarget(const QString& destPath, const QString& liveVaultPath) {
    const QString absDest = QFileInfo(destPath).absoluteFilePath();
    if (samePath(absDest, liveVaultPath)) {
        return true;
    }
    const QFileInfo live(liveVaultPath);
    const QString dir = live.absolutePath();
    return samePath(absDest, dir + QStringLiteral("/vault.restore.tmp"))
        || samePath(absDest, dir + QStringLiteral("/vault.grim.bak"));
}

} // namespace PathSafety
