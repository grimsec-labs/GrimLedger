#include "ui/CustomTitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>
#include <QWindow>

CustomTitleBar::CustomTitleBar(QWidget* parent)
    : QWidget(parent) {
    buildUi();

    if (QWidget* top = window()) {
        top->installEventFilter(this);
    }
}

void CustomTitleBar::buildUi() {
    setObjectName(QStringLiteral("CustomTitleBar"));
    setFixedHeight(36);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 4, 0);
    layout->setSpacing(8);

    auto* glyph = new QLabel(QStringLiteral("◆"), this);
    glyph->setObjectName(QStringLiteral("TitleBarGlyph"));

    m_titleLabel = new QLabel(QStringLiteral("GRIMLEDGER"), this);
    m_titleLabel->setObjectName(QStringLiteral("TitleBarTitle"));

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setObjectName(QStringLiteral("TitleBarSubtitle"));

    layout->addWidget(glyph);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_subtitleLabel);
    layout->addStretch();

    m_minButton = new QPushButton(QStringLiteral("─"), this);
    m_minButton->setObjectName(QStringLiteral("TitleBarButton"));
    m_minButton->setFixedSize(40, 28);
    m_minButton->setToolTip(QStringLiteral("Minimize"));
    connect(m_minButton, &QPushButton::clicked, this, &CustomTitleBar::onMinimize);

    m_maxButton = new QPushButton(QStringLiteral("□"), this);
    m_maxButton->setObjectName(QStringLiteral("TitleBarButton"));
    m_maxButton->setFixedSize(40, 28);
    m_maxButton->setToolTip(QStringLiteral("Maximize"));
    connect(m_maxButton, &QPushButton::clicked, this, &CustomTitleBar::onMaximizeRestore);

    m_closeButton = new QPushButton(QStringLiteral("✕"), this);
    m_closeButton->setObjectName(QStringLiteral("TitleBarClose"));
    m_closeButton->setFixedSize(40, 28);
    m_closeButton->setToolTip(QStringLiteral("Close"));
    connect(m_closeButton, &QPushButton::clicked, this, &CustomTitleBar::onClose);

    layout->addWidget(m_minButton);
    layout->addWidget(m_maxButton);
    layout->addWidget(m_closeButton);
}

void CustomTitleBar::setTitle(const QString& title) {
    m_titleLabel->setText(title);
}

void CustomTitleBar::setSubtitle(const QString& subtitle) {
    m_subtitleLabel->setText(subtitle);
    m_subtitleLabel->setVisible(!subtitle.isEmpty());
}

void CustomTitleBar::setMaximizeEnabled(bool enabled) {
    m_maximizeEnabled = enabled;
    m_maxButton->setVisible(enabled);
}

void CustomTitleBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (QWidget* top = window()) {
            if (QWindow* handle = top->windowHandle()) {
                handle->startSystemMove();
            }
        }
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent* event) {
    if (m_maximizeEnabled && event->button() == Qt::LeftButton) {
        onMaximizeRestore();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool CustomTitleBar::eventFilter(QObject* watched, QEvent* event) {
    if (watched == window() && event->type() == QEvent::WindowStateChange) {
        syncMaximizeButton();
    }
    return QWidget::eventFilter(watched, event);
}

void CustomTitleBar::onMinimize() {
    if (QWidget* top = window()) {
        top->showMinimized();
    }
}

void CustomTitleBar::onMaximizeRestore() {
    if (!m_maximizeEnabled) {
        return;
    }
    if (QWidget* top = window()) {
        if (top->isMaximized()) {
            top->showNormal();
        } else {
            top->showMaximized();
        }
        syncMaximizeButton();
    }
}

void CustomTitleBar::onClose() {
    if (QWidget* top = window()) {
        top->close();
    }
}

void CustomTitleBar::syncMaximizeButton() {
    if (!m_maxButton) {
        return;
    }
    if (QWidget* top = window()) {
        m_maxButton->setText(top->isMaximized()
            ? QStringLiteral("❐")
            : QStringLiteral("□"));
    }
}
