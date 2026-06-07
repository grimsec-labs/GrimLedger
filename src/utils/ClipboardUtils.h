#pragma once

#include <QString>

namespace ClipboardUtils {

void copyTextWithAutoClear(const QString& text, int clearAfterMs = 20000);

} // namespace ClipboardUtils
