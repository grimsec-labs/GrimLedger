#include "ui/GrimDialog.h"
#include "ui/CustomTitleBar.h"
#include "ui/FramelessResize.h"

#include <QGridLayout>
#include <QVBoxLayout>

GrimDialog::GrimDialog(const QString& windowTitle, QWidget* parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setObjectName(QStringLiteral("GrimDialog"));
    setModal(true);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_titleBar = new CustomTitleBar(this);
    m_titleBar->setTitle(QStringLiteral("GRIMLEDGER"));
    m_titleBar->setSubtitle(windowTitle);
    m_titleBar->setDialogMode(true);

    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("GrimDialogContent"));
    auto* inner = new QVBoxLayout(m_content);
    inner->setContentsMargins(16, 16, 16, 16);
    inner->setSpacing(12);

    outer->addWidget(m_titleBar);
    outer->addWidget(m_content, 1);
}

QVBoxLayout* GrimDialog::contentLayout() const {
    return qobject_cast<QVBoxLayout*>(m_content->layout());
}

QWidget* GrimDialog::contentWidget() const {
    return m_content;
}

void GrimDialog::injectTitleBar(QDialog* dialog, const QString& windowTitle) {
    if (!dialog || dialog->property("grimTitleInjected").toBool()) {
        return;
    }
    dialog->setProperty("grimTitleInjected", true);
    dialog->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    dialog->setObjectName(QStringLiteral("GrimDialog"));

    auto* bar = new CustomTitleBar(dialog);
    bar->setTitle(QStringLiteral("GRIMLEDGER"));
    bar->setSubtitle(windowTitle);
    bar->setDialogMode(true);

    QLayout* layout = dialog->layout();
    if (!layout) {
        return;
    }

    if (auto* grid = qobject_cast<QGridLayout*>(layout)) {
        struct GridEntry {
            int row = 0;
            int col = 0;
            int rowSpan = 1;
            int colSpan = 1;
            QLayoutItem* item = nullptr;
        };

        QList<GridEntry> entries;
        int maxCol = 0;
        for (int i = grid->count() - 1; i >= 0; --i) {
            GridEntry entry;
            grid->getItemPosition(i, &entry.row, &entry.col, &entry.rowSpan, &entry.colSpan);
            entry.item = grid->takeAt(i);
            maxCol = qMax(maxCol, entry.col + entry.colSpan);
            entries.prepend(entry);
        }

        const int span = qMax(1, maxCol);
        grid->addWidget(bar, 0, 0, 1, span);
        for (const GridEntry& entry : entries) {
            grid->addItem(entry.item, entry.row + 1, entry.col, entry.rowSpan, entry.colSpan);
        }
        return;
    }

    if (auto* vbox = qobject_cast<QVBoxLayout*>(layout)) {
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(0);
        vbox->insertWidget(0, bar);
    }
}

bool GrimDialog::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (FramelessResize::handleNativeEvent(this, eventType, message, result)) {
        return true;
    }
    return QDialog::nativeEvent(eventType, message, result);
}
