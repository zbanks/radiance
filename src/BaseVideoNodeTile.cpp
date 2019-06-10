#include "BaseVideoNodeTile.h"

BaseVideoNodeTile::BaseVideoNodeTile(QQuickItem *p)
    : QQuickItem(p)
{
    setFlag(QQuickItem::ItemIsFocusScope);
}

BaseVideoNodeTile::~BaseVideoNodeTile() = default;

VideoNodeSP *BaseVideoNodeTile::videoNode() const {
    return m_videoNode;
}
