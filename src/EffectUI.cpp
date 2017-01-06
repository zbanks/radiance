#include "EffectUI.h"
#include "main.h"

#include <QtCore/QMutex>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>

EffectUI::EffectUI() 
    : m_previous(0) {
    Effect *e = new Effect(renderContext);
    connect(e, &Effect::intensityChanged, this, &EffectUI::intensityChanged);
    m_videoNode = e;
}

qreal EffectUI::intensity() {
    return static_cast<Effect*>(m_videoNode)->intensity();
}

QString EffectUI::source() {
    return m_source;
}

VideoNodeUI *EffectUI::previous() {
    return m_previous;
}

void EffectUI::setIntensity(qreal value) {
    static_cast<Effect*>(m_videoNode)->setIntensity(value);
}

void EffectUI::setSource(QString value) {
    m_source = value;
    QQuickWindow *w = window();
    if(w) {
        QOpenGLContext *current = w->openglContext();
        if(current) {
            current->makeCurrent(w);
            static_cast<Effect*>(m_videoNode)->loadProgram(value);
        }
    }
    emit sourceChanged(value);
}

void EffectUI::setPrevious(VideoNodeUI *value) {
    m_previous = value;
    Effect *e = static_cast<Effect*>(m_videoNode);
    if(m_previous == NULL) {
        e->setPrevious(NULL);
    } else {
        e->setPrevious(value->m_videoNode);
    }
    emit previousChanged(value);
}

void EffectUI::initialize() {
    if(!m_source.isEmpty()) static_cast<Effect*>(m_videoNode)->loadProgram(m_source);
}
