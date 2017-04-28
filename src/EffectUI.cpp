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
    connect(this, &EffectUI::intensityInvoker,    e, &Effect::intensity);
    connect(this, &EffectUI::setIntensityInvoker, e, &Effect::setIntensity);
    connect(this, &EffectUI::setPreviousInvoker,  e, &Effect::setPrevious);

    connect(renderContext, &RenderContext::fpsChanged, this, &VideoNodeUI::setFps);
    m_videoNode = e;
}

QString EffectUI::source() {
    return m_source;
}

VideoNodeUI *EffectUI::previous() {
    return m_previous;
}

void EffectUI::setSource(QString value) {
    QQuickWindow *w = window();
    if(w != NULL) {
        // Apparently a window existing is good enough to ensure an OpenGL context
        bool success = static_cast<Effect*>(m_videoNode)->loadProgram(value);
        if(success) {
            m_source = value;
        }
        emit sourceChanged(value);
    } else {
        qWarning() << "setSource must be called from a valid OpenGL context!";
    }
}

void EffectUI::setPrevious(VideoNodeUI *value) {
    m_previous = value;
    Effect *e = static_cast<Effect*>(m_videoNode);
    if(m_previous == NULL) {
        emit setPreviousInvoker(NULL);
    } else {
        emit setPreviousInvoker(value->m_videoNode);
    }
    emit previousChanged(value);
}
