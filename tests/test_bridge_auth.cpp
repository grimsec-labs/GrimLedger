#include "bridge/BridgeAuth.h"

int main() {
    const QByteArray token = BridgeAuth::generateToken();
    if (token.size() < 16) {
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
    if (!BridgeAuth::constantTimeEquals(QByteArray("abc"), QByteArray("abc"))) {
        return 1;
    }
    if (BridgeAuth::constantTimeEquals(QByteArray("abc"), QByteArray("abd"))) {
        return 1;
    }
    BridgeAuth::clearSessionToken();
    return 0;
}
