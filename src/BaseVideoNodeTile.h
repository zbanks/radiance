#pragma once

#include <QQuickItem>
#include "Controls.h"

class BaseVideoNodeTile : public QQuickItem {
    Q_OBJECT

public:
    BaseVideoNodeTile(QQuickItem *p = nullptr);
   ~BaseVideoNodeTile() override;
};
