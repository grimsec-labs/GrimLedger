#pragma once

#include <QString>

namespace BreachCheck {

bool isEnabled();
void setEnabled(bool enabled);

struct Result {
    bool ok = false;
    bool breached = false;
    int count = 0;
    QString error;
};

Result checkPassword(const QString& password);

} // namespace BreachCheck
