#include "ui/GrimFileDialog.h"
#include "ui/GrimDialog.h"
#include "ui/FramelessResize.h"

#include <QDialog>
#include <QFileDialog>
#include <QShowEvent>

namespace {

class FramedFileDialog : public QFileDialog {
public:
    FramedFileDialog(
        QWidget* parent,
        const QString& title,
        const QString& caption,
        const QString& dir,
        const QString& filter)
        : QFileDialog(parent, caption, dir, filter)
        , m_title(title) {
        setOption(QFileDialog::DontUseNativeDialog, true);
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setObjectName(QStringLiteral("GrimDialog"));
    }

    FramedFileDialog(
        QWidget* parent,
        const QString& title,
        const QString& caption,
        const QString& dir,
        QFileDialog::Options options)
        : QFileDialog(parent, caption, dir, QString())
        , m_title(title) {
        setOptions(options | QFileDialog::DontUseNativeDialog);
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setObjectName(QStringLiteral("GrimDialog"));
    }

protected:
    void showEvent(QShowEvent* event) override {
        QFileDialog::showEvent(event);
        if (!m_framed) {
            GrimDialog::injectTitleBar(this, m_title);
            m_framed = true;
        }
    }

    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override {
        if (FramelessResize::handleNativeEvent(this, eventType, message, result)) {
            return true;
        }
        return QFileDialog::nativeEvent(eventType, message, result);
    }

private:
    QString m_title;
    bool m_framed = false;
};

} // namespace

QString GrimFileDialog::getOpenFileName(
    QWidget* parent,
    const QString& title,
    const QString& dir,
    const QString& filter) {
    FramedFileDialog dlg(parent, title, title, dir, filter);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.selectedFiles().value(0);
    }
    return QString();
}

QStringList GrimFileDialog::getOpenFileNames(
    QWidget* parent,
    const QString& title,
    const QString& dir,
    const QString& filter) {
    FramedFileDialog dlg(parent, title, title, dir, filter);
    dlg.setFileMode(QFileDialog::ExistingFiles);
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.selectedFiles();
    }
    return {};
}

QString GrimFileDialog::getSaveFileName(
    QWidget* parent,
    const QString& title,
    const QString& dir,
    const QString& filter) {
    FramedFileDialog dlg(parent, title, title, dir, filter);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.selectedFiles().value(0);
    }
    return QString();
}

QString GrimFileDialog::getExistingDirectory(
    QWidget* parent,
    const QString& title,
    const QString& dir) {
    FramedFileDialog dlg(
        parent,
        title,
        title,
        dir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setOption(QFileDialog::ShowDirsOnly, true);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.selectedFiles().value(0);
    }
    return QString();
}
