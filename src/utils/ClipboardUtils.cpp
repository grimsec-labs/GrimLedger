#include "utils/ClipboardUtils.h"

#include <QApplication>
#include <QClipboard>
#include <QTimer>
#include <QPointer>

namespace ClipboardUtils {

void copyTextWithAutoClear(const QString& text, int clearAfterMs) {
    if (text.isEmpty()) {
        return;
    }

    if (auto* clipboard = QApplication::clipboard()) {
        clipboard->setText(text);
    }

    const QPointer<QClipboard> clipboard = QApplication::clipboard();
    const QString captured = text;
    QTimer::singleShot(clearAfterMs, qApp, [clipboard, captured]() {
        if (!clipboard) {
            return;
        }
        if (clipboard->text() == captured) {
            clipboard->clear();
        }
    });
}

} // namespace ClipboardUtils
