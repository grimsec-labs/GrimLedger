#pragma once

#include <QByteArray>
#include <QString>

namespace BridgeAuth {

QString sessionDirectory();
QString tokenFilePath();
QString endpointName();

QByteArray generateToken();
bool writeSessionToken(const QByteArray& token);
bool readSessionToken(QByteArray& tokenOut);
void clearSessionToken();

bool constantTimeEquals(const QByteArray& a, const QByteArray& b);

} // namespace BridgeAuth
