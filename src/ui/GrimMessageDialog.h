#pragma once

#include "ui/GrimDialog.h"

class GrimMessageDialog : public GrimDialog {
    Q_OBJECT

public:
    enum class Type { Info, Warning, Critical, Question };

    GrimMessageDialog(
        Type type,
        const QString& title,
        const QString& text,
        QWidget* parent = nullptr);

    static void information(QWidget* parent, const QString& title, const QString& text);
    static void warning(QWidget* parent, const QString& title, const QString& text);
    static void critical(QWidget* parent, const QString& title, const QString& text);
    static bool question(QWidget* parent, const QString& title, const QString& text);
};
