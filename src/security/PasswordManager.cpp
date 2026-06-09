#include "security/PasswordManager.h"

#include <QRegularExpression>

PasswordManager::StrengthResult PasswordManager::evaluate(const QString& password) {
    StrengthResult result;
    int score = 0;

    if (password.isEmpty()) {
        result.label = QStringLiteral("Enter a password");
        result.color = QStringLiteral("#666666");
        return result;
    }

    const int len = password.size();
    if (len >= 8) score += 15;
    if (len >= 12) score += 15;
    if (len >= 16) score += 10;
    if (len >= 20) score += 10;

    if (password.contains(QRegularExpression(QStringLiteral("[a-z]")))) score += 10;
    if (password.contains(QRegularExpression(QStringLiteral("[A-Z]")))) score += 10;
    if (password.contains(QRegularExpression(QStringLiteral("[0-9]")))) score += 10;
    if (password.contains(QRegularExpression(QStringLiteral("[^a-zA-Z0-9]")))) score += 15;

    score = qBound(0, score, 100);
    result.score = score;

    if (score < 25) {
        result.level = Strength::VeryWeak;
        result.color = QStringLiteral("#8b0000");
    } else if (score < 45) {
        result.level = Strength::Weak;
        result.color = QStringLiteral("#cc3300");
    } else if (score < 65) {
        result.level = Strength::Fair;
        result.color = QStringLiteral("#ff6600");
    } else if (score < 85) {
        result.level = Strength::Strong;
        result.color = QStringLiteral("#33cc66");
    } else {
        result.level = Strength::VeryStrong;
        result.color = QStringLiteral("#00ff66");
    }

    result.label = strengthLabel(result.level);
    return result;
}

bool PasswordManager::isValidVaultPassword(const QString& password, QString* errorOut) {
    constexpr int kMinVaultPasswordLength = 12;
    if (password.size() < kMinVaultPasswordLength) {
        if (errorOut) {
            *errorOut = QStringLiteral("Master password must be at least 12 characters.");
        }
        return false;
    }
    return true;
}

QString PasswordManager::strengthLabel(Strength level) {
    switch (level) {
    case Strength::VeryWeak: return QStringLiteral("Very Weak");
    case Strength::Weak: return QStringLiteral("Weak");
    case Strength::Fair: return QStringLiteral("Fair");
    case Strength::Strong: return QStringLiteral("Strong");
    case Strength::VeryStrong: return QStringLiteral("Very Strong");
    }
    return QStringLiteral("Unknown");
}
