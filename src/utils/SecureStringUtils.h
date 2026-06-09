#pragma once

#include <QString>
#include <QByteArray>

class SecureStringUtils {
public:
    static void clearQString(QString& str);

    static QString fromUtf8Secure(const QByteArray& data);
    static QByteArray toUtf8Secure(const QString& str);
};
