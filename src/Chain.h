#pragma once

#include "OpenGLWorker.h"
#include "QmlSharedPointer.h"
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
    A Chain stores a particular resolution
    at which the render is to be done.
    A Chain also stores state that does not belong to any particular VideoNode
    for the render, i.e. a blank texture and noise texture
    which is available to all.

    Chains are immutable once created,
    that is, you cannot change the size.

    Chains are created by Outputs or Output-like things
    (such as the preview adapter.)

    Chains are thread-safe by nature of being immutable.
    However, OpenGL thread restrictions still apply
    (e.g. don't try to access the textures
    from multiple threads)
*/

class Chain : public QObject {
    Q_OBJECT

    Q_PROPERTY(QSize size READ size CONSTANT);
    Q_PROPERTY(int blankTexture READ blankTexture);
    Q_PROPERTY(int noiseTexture READ noiseTexture);

public:
    Chain(QSize size=QSize(0, 0));

    // Creates a new chain with the given size
    // to replace the given chain
    // (currently all this does
    // is move it to the other's thread)
    Chain(const Chain *other, QSize size);

    operator QString() const;

    bool operator==(const Chain &other) const;
    bool operator>(const Chain &other) const;
    bool operator<(const Chain &other) const;
    Chain &operator=(const Chain &other);

public slots:
    void moveToWorkerContext(OpenGLWorkerContext *context);
    QSize size() const;
    GLuint noiseTexture();
    GLuint blankTexture();
    QOpenGLVertexArrayObject *vao();

protected:
    QOpenGLTexture m_noiseTexture;
    QOpenGLTexture m_blankTexture;
    QOpenGLVertexArrayObject m_vao{};
    QSize m_size{};
};

typedef QmlSharedPointer<Chain> ChainSP;
Q_DECLARE_METATYPE(ChainSP*)
