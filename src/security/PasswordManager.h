#pragma once

#include <QString>

// Password strength evaluation and validation helpers.
// Does not store passwords.
class PasswordManager {
public:
    enum class Strength { VeryWeak, Weak, Fair, Strong, VeryStrong };

    struct StrengthResult {
        Strength level = Strength::VeryWeak;
        int score = 0;       // 0-100
        QString label;
        QString color;       // hex accent for UI meter
    };

    static StrengthResult evaluate(const QString& password);
    static bool isValidVaultPassword(const QString& password, QString* errorOut = nullptr);
    static QString strengthLabel(Strength level);
};
