#include "EffectUI.h"
#include "main.h"

#include <QtCore/QMutex>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>

EffectUI::EffectUI(QString source)
    : m_previous(0)
    , m_source(source) {
    Effect *e = new Effect(renderContext);
    connect(e, &Effect::intensityChanged, this, &EffectUI::intensityChanged);
    m_videoNode = e;
    connect(e, &VideoNode::initialized, this, &EffectUI::onInitialized, Qt::DirectConnection);
}

void EffectUI::onInitialized() {
    if(!m_source.isEmpty()) static_cast<Effect*>(m_videoNode)->loadProgram(m_source);
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
    if(w != NULL) {
        // Apparently a window existing is good enough to ensure an OpenGL context
        bool success = static_cast<Effect*>(m_videoNode)->loadProgram(value);
        if (!success) {
            m_source = value = nullptr;
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
