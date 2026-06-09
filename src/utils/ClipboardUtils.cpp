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
        auto* mime = new QMimeData();
        mime->setText(text);
        const QByteArray zeroDword(4, '\0');
        mime->setData(QStringLiteral("ExcludeClipboardContentFromMonitorProcessing"), zeroDword);
        mime->setData(QStringLiteral("CanIncludeInClipboardHistory"), zeroDword);
        mime->setData(QStringLiteral("CanUploadToCloudClipboard"), zeroDword);
        clipboard->setMimeData(mime);
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
