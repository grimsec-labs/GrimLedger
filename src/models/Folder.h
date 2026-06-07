#pragma once

#include <QString>

struct Folder {
    qint64 id = 0;
    QString name;
    qint64 parentId = 0;
};
