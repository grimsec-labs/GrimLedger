// Tests for FillTrustLevel: default, fromInt, labels, and permission queries.

#include "models/FillTrustLevel.h"
#include "models/Credential.h"

#include <QCoreApplication>
#include <QString>

namespace {

int fail(const char* what) {
    qWarning("test_fill_trust_level FAILED: %s", what);
    return 1;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // --- 1. Default credential trust level is ExactOrigin ---
    {
        Credential c;
        if (c.fillTrustLevel != FillTrustLevel::ExactOrigin)
            return fail("default fillTrustLevel should be ExactOrigin");
    }

    // --- 2. fillTrustLevelFromInt round-trips valid values ---
    {
        if (fillTrustLevelFromInt(0) != FillTrustLevel::ManualOnly)
            return fail("fromInt(0) != ManualOnly");
        if (fillTrustLevelFromInt(1) != FillTrustLevel::UsernameOnly)
            return fail("fromInt(1) != UsernameOnly");
        if (fillTrustLevelFromInt(2) != FillTrustLevel::ExactOrigin)
            return fail("fromInt(2) != ExactOrigin");
        if (fillTrustLevelFromInt(3) != FillTrustLevel::AllowSubdomains)
            return fail("fromInt(3) != AllowSubdomains");
    }

    // --- 3. Out-of-range int falls back to ExactOrigin ---
    {
        if (fillTrustLevelFromInt(-1) != FillTrustLevel::ExactOrigin)
            return fail("fromInt(-1) should fall back to ExactOrigin");
        if (fillTrustLevelFromInt(99) != FillTrustLevel::ExactOrigin)
            return fail("fromInt(99) should fall back to ExactOrigin");
    }

    // --- 4. Labels are non-empty for all levels ---
    {
        for (int i = 0; i <= 3; ++i) {
            const auto label = fillTrustLevelLabel(fillTrustLevelFromInt(i));
            if (label.isEmpty())
                return fail("label should be non-empty for all valid levels");
        }
    }

    // --- 5. ManualOnly blocks bridge listing ---
    {
        if (fillTrustAllowsBridgeListing(FillTrustLevel::ManualOnly))
            return fail("ManualOnly should block bridge listing");
    }

    // --- 6. ExactOrigin allows bridge listing ---
    {
        if (!fillTrustAllowsBridgeListing(FillTrustLevel::ExactOrigin))
            return fail("ExactOrigin should allow bridge listing");
    }

    // --- 7. UsernameOnly does not send password ---
    {
        if (fillTrustSendsPassword(FillTrustLevel::UsernameOnly))
            return fail("UsernameOnly should not send password");
    }

    // --- 8. ExactOrigin sends password ---
    {
        if (!fillTrustSendsPassword(FillTrustLevel::ExactOrigin))
            return fail("ExactOrigin should send password");
    }

    // --- 9. AllowSubdomains allows subdomains ---
    {
        if (!fillTrustAllowsSubdomains(FillTrustLevel::AllowSubdomains))
            return fail("AllowSubdomains should allow subdomains");
        if (fillTrustAllowsSubdomains(FillTrustLevel::ExactOrigin))
            return fail("ExactOrigin should not allow subdomains");
    }

    return 0;
}
