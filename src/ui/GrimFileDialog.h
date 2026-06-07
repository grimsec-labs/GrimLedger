#pragma once

#include <QString>
#include <QStringList>

class QWidget;

class GrimFileDialog {
public:
    static QString getOpenFileName(
        QWidget* parent,
        const QString& title,
        const QString& dir = QString(),
        const QString& filter = QString());

    static QStringList getOpenFileNames(
        QWidget* parent,
        const QString& title,
        const QString& dir = QString(),
        const QString& filter = QString());

    static QString getSaveFileName(
        QWidget* parent,
        const QString& title,
        const QString& dir = QString(),
        const QString& filter = QString());

    static QString getExistingDirectory(
        QWidget* parent,
        const QString& title,
        const QString& dir = QString());
};
