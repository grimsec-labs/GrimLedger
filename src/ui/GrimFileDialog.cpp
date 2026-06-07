#include "ui/GrimFileDialog.h"
#include "ui/GrimDialog.h"
#include "ui/FramelessResize.h"

#include <QDialog>
#include <QFileDialog>

namespace {

class FramedFileDialog : public QFileDialog {
public:
    FramedFileDialog(
        QWidget* parent,
        const QString& title,
        const QString& caption,
        const QString& dir,
        const QString& filter,
        const QString& subtitle = QString())
        : QFileDialog(parent, caption, dir, filter)
        , m_title(title)
        , m_subtitle(subtitle) {
        setOption(QFileDialog::DontUseNativeDialog, true);
        setModal(true);
        setMinimumSize(700, 500);
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
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
        setModal(true);
        setMinimumSize(700, 500);
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setObjectName(QStringLiteral("GrimDialog"));
    }

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override {
        if (FramelessResize::handleNativeEvent(this, eventType, message, result)) {
            return true;
        }
        return QFileDialog::nativeEvent(eventType, message, result);
    }

private:
    QString m_title;
    QString m_subtitle;
};

int runFramedFileDialog(
    FramedFileDialog& dlg,
    const QString& title,
    const QString& subtitle) {
    GrimDialog::injectTitleBar(&dlg, title, subtitle);
    if (auto* parent = dlg.parentWidget()) {
        const QRect parentGeo = parent->frameGeometry();
        dlg.adjustSize();
        dlg.move(parentGeo.center() - dlg.rect().center());
    }
    dlg.raise();
    dlg.activateWindow();
    return dlg.exec();
}

} // namespace

QString GrimFileDialog::getOpenFileName(
    QWidget* parent,
    const QString& title,
    const QString& dir,
    const QString& filter,
    const QString& subtitle) {
    FramedFileDialog dlg(parent, title, title, dir, filter, subtitle);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    if (runFramedFileDialog(dlg, title, subtitle) == QDialog::Accepted) {
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
    if (runFramedFileDialog(dlg, title, QString()) == QDialog::Accepted) {
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
    if (runFramedFileDialog(dlg, title, QString()) == QDialog::Accepted) {
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
    if (runFramedFileDialog(dlg, title, QString()) == QDialog::Accepted) {
        return dlg.selectedFiles().value(0);
    }
    return QString();
}
