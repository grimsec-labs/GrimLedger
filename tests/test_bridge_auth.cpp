#include "bridge/BridgeAuth.h"

#include <sodium.h>

int main() {
    if (sodium_init() < 0) {
        return 1;
    }
    const QByteArray token = BridgeAuth::generateToken();
    if (token.size() != 32) {
        return 1;
    }
    if (!BridgeAuth::writeSessionToken(token)) {
        return 1;
    }
    QByteArray readBack;
    if (!BridgeAuth::readSessionToken(readBack)) {
        return 1;
    }
    if (!BridgeAuth::constantTimeEquals(token, readBack)) {
        return 1;
    }

    const QString transport = BridgeAuth::tokenToTransportString(token);
    if (transport.size() != 64) {
        return 1;
    }
    const auto decoded = BridgeAuth::tokenFromTransportString(transport);
    if (!decoded || !BridgeAuth::constantTimeEquals(token, *decoded)) {
        return 1;
    }
    if (BridgeAuth::tokenFromTransportString(QStringLiteral("not-hex"))) {
        return 1;
    }

    if (!BridgeAuth::constantTimeEquals(QByteArray("abc"), QByteArray("abc"))) {
        return 1;
    }
    if (BridgeAuth::constantTimeEquals(QByteArray("abc"), QByteArray("abd"))) {
        return 1;
    }
    BridgeAuth::clearSessionToken();
    return 0;
}
