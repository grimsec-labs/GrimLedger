#include "utils/DialogUtils.h"
#include "ui/GrimMessageDialog.h"

void DialogUtils::warning(QWidget* parent, const QString& title, const QString& text) {
    GrimMessageDialog::warning(parent, title, text);
}

void DialogUtils::information(QWidget* parent, const QString& title, const QString& text) {
    GrimMessageDialog::information(parent, title, text);
}

void DialogUtils::critical(QWidget* parent, const QString& title, const QString& text) {
    GrimMessageDialog::critical(parent, title, text);
}

bool DialogUtils::question(QWidget* parent, const QString& title, const QString& text) {
    return GrimMessageDialog::question(parent, title, text);
}
