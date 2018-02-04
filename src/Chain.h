#pragma once

#include "OpenGLWorker.h"
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QSharedPointer>
#include <QVector>
#include <QMutex>

/*
    Chains are instances of the
    model render pipeline
    You need a different chain for a different size / shape output,
    or a different thead.
    When requesting a render of the model,
    you must use one of its chains.

    All VideoNode renders happen against a chain.
    A Chain is lightweight object that stores a particular resolution
    at which the render is to be done.
    A Chain also stores state that does not belong to any particular VideoNode
    for the render, i.e. a blank texture and noise texture
    which is available to all.

    Chains are immutable once created,
    that is, you cannot change the size.
    Chains should always be passed around as Shared Pointers.
    This is because they cannot be copied due to having some OpenGL baggage,
    yet they may be referenced by VideoNodes that have been
    copied for rendering.

    Chains are owned by Outputs or Output-like things
    (such as the preview adapter.)
*/

class Chain : public QObject {
    Q_OBJECT

    Q_PROPERTY(QSize size READ size CONSTANT);
    Q_PROPERTY(int blankTexture READ blankTexture);
    Q_PROPERTY(int noiseTexture READ noiseTexture);

public:
    Chain(QSize size);
   ~Chain() override;
    QSize size();

public slots:
    GLuint noiseTexture();
    GLuint blankTexture();
    QOpenGLVertexArrayObject &vao();

protected:
    QOpenGLTexture m_noiseTexture;
    QOpenGLTexture m_blankTexture;
    QOpenGLVertexArrayObject  m_vao;
    QSize m_size;
};
