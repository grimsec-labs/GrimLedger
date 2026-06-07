#pragma once

#include <QString>
#include <QByteArray>

class SecureStringUtils {
public:
    // Overwrite QString internal buffer best-effort (Qt strings are immutable;
    // we work with UTF-8 byte buffers for sensitive data where possible).
    static void clearQString(QString& str);

    static QString fromUtf8Secure(const QByteArray& data);
    static QByteArray toUtf8Secure(const QString& str);
};
