#pragma once

#include <QtGlobal>

namespace SecurityLimits {
    constexpr qint64 kMaxBackupFileBytes = 256LL * 1024 * 1024;
    constexpr qint64 kMaxDatabaseBytes = 200LL * 1024 * 1024;
    constexpr qint64 kMaxMarkdownImportBytes = 10LL * 1024 * 1024;
    constexpr qint64 kMaxVaultAttachmentBytes = 100LL * 1024 * 1024;
}
