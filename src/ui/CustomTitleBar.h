#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

// Custom infernal-themed title bar for frameless windows.
class CustomTitleBar : public QWidget {
    Q_OBJECT

public:
    explicit CustomTitleBar(QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);
    void setMaximizeEnabled(bool enabled);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onMinimize();
    void onMaximizeRestore();
    void onClose();
    void syncMaximizeButton();

private:
    void buildUi();

    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QPushButton* m_minButton = nullptr;
    QPushButton* m_maxButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    bool m_maximizeEnabled = true;
};
