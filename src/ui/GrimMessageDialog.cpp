#include "ui/GrimMessageDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace {

QString iconForType(GrimMessageDialog::Type type) {
    switch (type) {
    case GrimMessageDialog::Type::Info: return QStringLiteral("ℹ");
    case GrimMessageDialog::Type::Warning: return QStringLiteral("⚠");
    case GrimMessageDialog::Type::Critical: return QStringLiteral("✖");
    case GrimMessageDialog::Type::Question: return QStringLiteral("?");
    }
    return QString();
}

} // namespace

GrimMessageDialog::GrimMessageDialog(
    Type type,
    const QString& title,
    const QString& text,
    QWidget* parent)
    : GrimDialog(title, parent) {
    setMinimumWidth(360);

    auto* row = new QHBoxLayout();
    auto* icon = new QLabel(iconForType(type), this);
    icon->setObjectName(QStringLiteral("DialogIcon"));
    icon->setFixedWidth(28);

    auto* message = new QLabel(text, this);
    message->setObjectName(QStringLiteral("DialogMessage"));
    message->setWordWrap(true);

    row->addWidget(icon);
    row->addWidget(message, 1);
    contentLayout()->addLayout(row);

    auto* buttons = new QHBoxLayout();
    buttons->addStretch();

    if (type == Type::Question) {
        auto* noBtn = new QPushButton(QStringLiteral("No"), this);
        noBtn->setObjectName(QStringLiteral("SecondaryButton"));
        connect(noBtn, &QPushButton::clicked, this, &GrimMessageDialog::reject);
        buttons->addWidget(noBtn);

        auto* yesBtn = new QPushButton(QStringLiteral("Yes"), this);
        yesBtn->setObjectName(QStringLiteral("PrimaryButton"));
        connect(yesBtn, &QPushButton::clicked, this, &GrimMessageDialog::accept);
        buttons->addWidget(yesBtn);
    } else {
        auto* okBtn = new QPushButton(QStringLiteral("OK"), this);
        okBtn->setObjectName(QStringLiteral("PrimaryButton"));
        connect(okBtn, &QPushButton::clicked, this, &GrimMessageDialog::accept);
        buttons->addWidget(okBtn);
    }

    contentLayout()->addLayout(buttons);
}

void GrimMessageDialog::information(QWidget* parent, const QString& title, const QString& text) {
    GrimMessageDialog dlg(Type::Info, title, text, parent);
    dlg.exec();
}

void GrimMessageDialog::warning(QWidget* parent, const QString& title, const QString& text) {
    GrimMessageDialog dlg(Type::Warning, title, text, parent);
    dlg.exec();
}

void GrimMessageDialog::critical(QWidget* parent, const QString& title, const QString& text) {
    GrimMessageDialog dlg(Type::Critical, title, text, parent);
    dlg.exec();
}

bool GrimMessageDialog::question(QWidget* parent, const QString& title, const QString& text) {
    GrimMessageDialog dlg(Type::Question, title, text, parent);
    return dlg.exec() == QDialog::Accepted;
}
