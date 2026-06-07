#include "bridge/BridgeAuth.h"

#include <sodium.h>

#include <QString>

namespace {

bool check(bool condition) {
    return condition;
}

} // namespace

int main() {
    if (sodium_init() < 0) {
        return 1;
    }

    const QByteArray token = BridgeAuth::generateToken();
    if (!check(token.size() == 32)) {
        return 1;
    }

    const QString transport = BridgeAuth::tokenToTransportString(token);
    if (!check(transport.size() == 64)) {
        return 1;
    }

    const auto decoded = BridgeAuth::tokenFromTransportString(transport);
    if (!check(decoded.has_value())) {
        return 1;
    }
    if (!check(BridgeAuth::constantTimeEquals(token, *decoded))) {
        return 1;
    }

    const QString nonceA = QStringLiteral("11111111-1111-1111-1111-111111111111");
    const QString nonceB = QStringLiteral("22222222-2222-2222-2222-222222222222");
    if (!check(nonceA != nonceB)) {
        return 1;
    }

    return 0;
}
