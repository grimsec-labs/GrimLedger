#pragma once

#include <QString>

namespace DiagramRenderer {

bool isDiagramLanguage(const QString& lang);
QString renderMermaidSubset(const QString& source, const QString& accentColor);

} // namespace DiagramRenderer
