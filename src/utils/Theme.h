#pragma once

#include <QString>

class QApplication;

class Theme {
public:
    static inline const QString kDefaultAccent = QStringLiteral("#cc2200");

    static QString loadStylesheet(const QString& accentHex = kDefaultAccent);
    static void apply(QApplication& app, const QString& accentHex = kDefaultAccent);

    static QString savedAccent();
    static void saveAccent(const QString& accentHex);
};
