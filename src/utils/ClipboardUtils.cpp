#include "utils/ClipboardUtils.h"

#include <QApplication>
#include <QClipboard>
#include <QTimer>

namespace ClipboardUtils {

void copyTextWithAutoClear(const QString& text, int clearAfterMs) {
    if (text.isEmpty()) {
        return;
    }
    QClipboard* clip = QApplication::clipboard();
    if (!clip) {
        return;
    }
    clip->setText(text);

    QTimer::singleShot(clearAfterMs, clip, [clip, text]() {
        if (clip->text() == text) {
            clip->clear();
        }
    });
}

} // namespace ClipboardUtils
