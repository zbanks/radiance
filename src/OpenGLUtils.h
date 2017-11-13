#pragma once

#include <QObject>
#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QSharedPointer>

#include "OpenGLWorkerContext.h"

using QOGLShaderProgramPointer = QSharedPointer<QOpenGLShaderProgram>;
using QOGLFramebufferObjectPointer = QSharedPointer<QOpenGLFramebufferObject>;
using QOGLTexturePointer = QSharedPointer<QOpenGLTexture>;
using QOGLBufferPointer = QSharedPointer<QOpenGLBuffer>;

class OpenGLSampler {
    Q_GADGET
public:
    constexpr OpenGLSampler() = default;
    OpenGLSampler(OpenGLSampler && o) noexcept = default;
    OpenGLSampler &operator=(OpenGLSampler && o) noexcept = default;
   ~OpenGLSampler();
    operator const GLuint& () const
    {
        return m_id;
    }
    bool isCreated() const
    {
        return m_id != 0u;
    }
    bool create();
    void destroy();
    GLuint samplerId() const
    {
        return m_id;
    };
    void setParameter(GLenum pname, GLint param);
    void setParameter(GLenum pname, GLfloat param);
    void bind(GLuint where);
protected:
    GLuint m_id{0u};
};

struct Pass {
    QOGLFramebufferObjectPointer m_output;
    QOGLShaderProgramPointer     m_shader;
};

QOGLShaderProgramPointer copyProgram(QOGLShaderProgramPointer program);
