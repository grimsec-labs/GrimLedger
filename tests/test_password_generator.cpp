// Tests for PasswordGenerator: length, character classes.

#include "utils/PasswordGenerator.h"

#include <QCoreApplication>
#include <QString>
#include <QSet>

namespace {

int fail(const char* what) {
    qWarning("test_password_generator FAILED: %s", what);
    return 1;
}

bool containsAny(const QString& s, const QString& chars) {
    for (const QChar& c : s) {
        if (chars.contains(c)) return true;
    }
    return false;
}

bool containsOnly(const QString& s, const QString& allowed) {
    for (const QChar& c : s) {
        if (!allowed.contains(c)) return false;
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // --- 1. Default generate produces correct length ---
    {
        auto pw = PasswordGenerator::generate(20);
        if (pw.length() != 20) return fail("default generate(20) wrong length");
    }

    // --- 2. Minimum length clamped to 8 ---
    {
        auto pw = PasswordGenerator::generate(3);
        if (pw.length() != 8) return fail("generate(3) should clamp to 8");
    }

    // --- 3. Maximum length clamped to 128 ---
    {
        auto pw = PasswordGenerator::generate(200);
        if (pw.length() != 128) return fail("generate(200) should clamp to 128");
    }

    // --- 4. Output only contains alphabet characters ---
    {
        const QString alphabet =
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789"
            "!@#$%^&*()-_=+[]{}:,.?";
        for (int i = 0; i < 10; ++i) {
            auto pw = PasswordGenerator::generate(32);
            if (!containsOnly(pw, alphabet))
                return fail("generated password contains unexpected character");
        }
    }

    // --- 5. Uniqueness: 100 passwords should all be different ---
    {
        QSet<QString> seen;
        for (int i = 0; i < 100; ++i) {
            seen.insert(PasswordGenerator::generate(20));
        }
        if (seen.size() < 95)
            return fail("too many duplicate passwords generated (entropy concern)");
    }

    return 0;
}
