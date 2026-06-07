#pragma once

#include <QString>

class QApplication;

class Theme {
public:
    static QString loadStylesheet(const QString& accentHex = QStringLiteral("#cc2200"));
    static void apply(QApplication& app, const QString& accentHex = QStringLiteral("#cc2200"));
};
