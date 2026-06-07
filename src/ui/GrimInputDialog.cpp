#include "ui/GrimInputDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

GrimInputDialog::GrimInputDialog(
    const QString& title,
    const QString& label,
    QWidget* parent,
    bool passwordMode)
    : GrimDialog(title, parent) {
    setMinimumWidth(360);

    auto* prompt = new QLabel(label, this);
    prompt->setObjectName(QStringLiteral("DialogPrompt"));

    auto* input = new QLineEdit(this);
    input->setObjectName(QStringLiteral("TitleField"));
    if (passwordMode) {
        input->setEchoMode(QLineEdit::Password);
    }
    m_lineEdit = input;

    auto* buttons = new QHBoxLayout();
    buttons->addStretch();

    auto* cancelBtn = new QPushButton(QStringLiteral("Cancel"), this);
    cancelBtn->setObjectName(QStringLiteral("SecondaryButton"));
    connect(cancelBtn, &QPushButton::clicked, this, &GrimInputDialog::reject);

    auto* okBtn = new QPushButton(QStringLiteral("OK"), this);
    okBtn->setObjectName(QStringLiteral("PrimaryButton"));
    connect(okBtn, &QPushButton::clicked, this, &GrimInputDialog::accept);
    connect(input, &QLineEdit::returnPressed, this, &GrimInputDialog::accept);

    buttons->addWidget(cancelBtn);
    buttons->addWidget(okBtn);

    contentLayout()->addWidget(prompt);
    contentLayout()->addWidget(input);
    contentLayout()->addLayout(buttons);
}

QString GrimInputDialog::text() const {
    return m_lineEdit ? m_lineEdit->text() : QString();
}

QString GrimInputDialog::getText(
    QWidget* parent,
    const QString& title,
    const QString& label,
    bool* ok) {
    GrimInputDialog dlg(title, label, parent);
    const int result = dlg.exec();
    const bool accepted = result == QDialog::Accepted;
    if (ok) {
        *ok = accepted;
    }
    return accepted ? dlg.text() : QString();
}

QString GrimInputDialog::getPassword(
    QWidget* parent,
    const QString& title,
    const QString& label,
    bool* ok) {
    GrimInputDialog dlg(title, label, parent, true);
    const int result = dlg.exec();
    const bool accepted = result == QDialog::Accepted;
    if (ok) {
        *ok = accepted;
    }
    return accepted ? dlg.text() : QString();
}
