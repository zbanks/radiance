#pragma once
#include <QtQuick/QQuickFramebufferObject>

class WaveformUI : public QQuickFramebufferObject
{
    Q_OBJECT
    QQuickFramebufferObject::Renderer *createRenderer() const;
};
