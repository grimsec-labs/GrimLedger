#pragma once

#include "ui/GrimDialog.h"

class QLineEdit;

class GrimInputDialog : public GrimDialog {
    Q_OBJECT

public:
    explicit GrimInputDialog(
        const QString& title,
        const QString& label,
        QWidget* parent = nullptr,
        bool passwordMode = false);

    QString text() const;

    static QString getText(
        QWidget* parent,
        const QString& title,
        const QString& label,
        bool* ok = nullptr);

    static QString getPassword(
        QWidget* parent,
        const QString& title,
        const QString& label,
        bool* ok = nullptr);

private:
    QLineEdit* m_lineEdit = nullptr;
};
