#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

namespace BridgeAuth {

QString sessionDirectory();
QString tokenFilePath();
QString endpointName();
QString generateSessionEndpoint();
bool writeSessionEndpoint(const QString& endpoint);
bool readSessionEndpoint(QString& endpointOut);
void clearSessionEndpoint();

QByteArray generateToken();
QString tokenToTransportString(const QByteArray& token);
std::optional<QByteArray> tokenFromTransportString(const QString& encoded);
bool writeSessionToken(const QByteArray& token);
bool readSessionToken(QByteArray& tokenOut);
void clearSessionToken();

bool constantTimeEquals(const QByteArray& a, const QByteArray& b);

} // namespace BridgeAuth
