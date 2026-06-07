#pragma once

#include <QString>
#include <QByteArray>

namespace TotpGenerator {

QString normalizeSecret(const QString& input);
bool isValidSecret(const QString& secret);
QString currentCode(const QString& secret, int periodSeconds = 30, int digits = 6);
int secondsRemaining(int periodSeconds = 30);

} // namespace TotpGenerator
