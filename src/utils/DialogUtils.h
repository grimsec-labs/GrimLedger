#pragma once

#include <QString>

class QWidget;

namespace DialogUtils {
    void warning(QWidget* parent, const QString& title, const QString& text);
    void information(QWidget* parent, const QString& title, const QString& text);
    void critical(QWidget* parent, const QString& title, const QString& text);
    bool question(QWidget* parent, const QString& title, const QString& text);
}
