#include "utils/Theme.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QSettings>

QString Theme::loadStylesheet(const QString& accentHex) {
    QFile styleFile(QStringLiteral(":/styles/grimledger_dark.qss"));
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QString css = QString::fromUtf8(styleFile.readAll());
    const QColor accent(accentHex);
    const QColor accentHover = accent.lighter(125);
    const QColor accentSoft = accent.darker(150);

    css.replace(QStringLiteral("@@ACCENT@@"), accentHex);
    css.replace(QStringLiteral("@@ACCENT_HOVER@@"), accentHover.name());
    css.replace(QStringLiteral("@@ACCENT_SOFT@@"), accentSoft.name());
    return css;
}

void Theme::apply(QApplication& app, const QString& accentHex) {
    app.setStyleSheet(loadStylesheet(accentHex));
}

QString Theme::savedAccent() {
    QSettings settings;
    return settings.value(QStringLiteral("appearance/accent"), kDefaultAccent).toString();
}

void Theme::saveAccent(const QString& accentHex) {
    QSettings settings;
    settings.setValue(QStringLiteral("appearance/accent"), accentHex);
}
