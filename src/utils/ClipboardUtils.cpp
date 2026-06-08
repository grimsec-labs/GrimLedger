#include "utils/ClipboardUtils.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QTimer>
#include <QPointer>

namespace ClipboardUtils {

void copyTextWithAutoClear(const QString& text, int clearAfterMs) {
    if (text.isEmpty()) {
        return;
    }

    if (auto* clipboard = QApplication::clipboard()) {
        // GL-SEC-008: mark the payload so Windows keeps it out of Clipboard History
        // (Win+V) and the cloud clipboard, in addition to the timed clear below.
        // These format names are inert on other platforms.
        auto* mime = new QMimeData();
        mime->setText(text);
        const QByteArray zeroDword(4, '\0');
        mime->setData(QStringLiteral("ExcludeClipboardContentFromMonitorProcessing"), zeroDword);
        mime->setData(QStringLiteral("CanIncludeInClipboardHistory"), zeroDword);
        mime->setData(QStringLiteral("CanUploadToCloudClipboard"), zeroDword);
        clipboard->setMimeData(mime);  // clipboard takes ownership
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
