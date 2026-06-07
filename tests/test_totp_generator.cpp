#include "utils/TotpGenerator.h"

#include <sodium.h>

#include <QCoreApplication>
#include <QString>

namespace {

bool check(bool condition) {
    return condition;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    if (sodium_init() < 0) {
        return 1;
    }

    const QString secret = QStringLiteral("JBSWY3DPEHPK3PXP");
    if (!check(TotpGenerator::isValidSecret(secret))) {
        return 1;
    }

    const QString code = TotpGenerator::currentCode(secret);
    if (!check(code.size() == 6)) {
        return 1;
    }
    if (!check(code == TotpGenerator::currentCode(secret))) {
        return 1;
    }
    if (!check(TotpGenerator::secondsRemaining() > 0)) {
        return 1;
    }

    return 0;
}
