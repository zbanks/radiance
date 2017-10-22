#include "BaseVideoNodeTile.h"

BaseVideoNodeTile::BaseVideoNodeTile(QQuickItem *p)
    : QQuickItem(p)
{
    setFlag(QQuickItem::ItemIsFocusScope);
}

BaseVideoNodeTile::~BaseVideoNodeTile() = default;
