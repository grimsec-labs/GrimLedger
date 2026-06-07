#pragma once

#include <QString>

enum class ChamberId : int {
    General = 0,
    Credentials = 1,
    Work = 2,
    Personal = 3,
    Casebook = 4,
};

QString chamberLabel(ChamberId chamber);
int chamberIdValue(ChamberId chamber);
