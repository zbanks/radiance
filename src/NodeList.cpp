#include "NodeList.h"

#include <QDir>

QStringList NodeList::effectNames() {
    auto filters = QStringList{} << QString{"*.0.glsl"};
    QDir dir("../resources/effects/");
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);
    return dir.entryList().replaceInStrings(".0.glsl","");
}
