#pragma once

#include <QDialog>

class QVBoxLayout;
class QWidget;
class CustomTitleBar;

// Frameless dialog shell with the GrimLedger custom title bar.
class GrimDialog : public QDialog {
    Q_OBJECT

public:
    explicit GrimDialog(const QString& windowTitle, QWidget* parent = nullptr);

    QVBoxLayout* contentLayout() const;
    QWidget* contentWidget() const;
    CustomTitleBar* titleBar() const { return m_titleBar; }

    // Inject a custom title bar into an existing QDialog (e.g. QFileDialog).
    static void injectTitleBar(
        QDialog* dialog,
        const QString& windowTitle,
        const QString& subtitle = QString());

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    CustomTitleBar* m_titleBar = nullptr;
    QWidget* m_content = nullptr;
};
