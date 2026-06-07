#pragma once

#include <QFileInfo>
#include <QString>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace PathSafety {

inline bool sharesHardLink(const QString& a, const QString& b) {
    if (a.isEmpty() || b.isEmpty()) {
        return false;
    }
#ifdef _WIN32
    const HANDLE ha = CreateFileW(
        reinterpret_cast<LPCWSTR>(a.utf16()),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);
    const HANDLE hb = CreateFileW(
        reinterpret_cast<LPCWSTR>(b.utf16()),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);
    if (ha == INVALID_HANDLE_VALUE || hb == INVALID_HANDLE_VALUE) {
        if (ha != INVALID_HANDLE_VALUE) {
            CloseHandle(ha);
        }
        if (hb != INVALID_HANDLE_VALUE) {
            CloseHandle(hb);
        }
        return false;
    }

    BY_HANDLE_FILE_INFORMATION infoA{};
    BY_HANDLE_FILE_INFORMATION infoB{};
    const bool ok = GetFileInformationByHandle(ha, &infoA) != 0
        && GetFileInformationByHandle(hb, &infoB) != 0
        && infoA.dwVolumeSerialNumber == infoB.dwVolumeSerialNumber
        && infoA.nFileIndexHigh == infoB.nFileIndexHigh
        && infoA.nFileIndexLow == infoB.nFileIndexLow;
    CloseHandle(ha);
    CloseHandle(hb);
    return ok;
#else
    Q_UNUSED(a);
    Q_UNUSED(b);
    return false;
#endif
}

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
    if (samePath(absDest, liveVaultPath) || sharesHardLink(absDest, liveVaultPath)) {
        return true;
    }
    const QFileInfo live(liveVaultPath);
    const QString dir = live.absolutePath();
    return samePath(absDest, dir + QStringLiteral("/vault.restore.tmp"))
        || samePath(absDest, dir + QStringLiteral("/vault.grim.bak"));
}

} // namespace PathSafety
