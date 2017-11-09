#pragma once

#include <QObject>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QSharedPointer>

#include "OpenGLWorkerContext.h"

class OpenGLSampler {
    Q_GADGET
public:
    constexpr OpenGLSampler() = default;
    OpenGLSampler(OpenGLSampler && o) noexcept = default;
    OpenGLSampler &operator=(OpenGLSampler && o) noexcept = default;
   ~OpenGLSampler();
    constexpr operator const GLuint& () const
    {
        return m_id;
    }
    constexpr bool isCreated() const
    {
        return m_id != 0u;
    }
    bool create();
    void destroy();
    constexpr GLuint samplerId() const
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
    QSharedPointer<QOpenGLFramebufferObject> m_output;
    QSharedPointer<QOpenGLShaderProgram>     m_shader;
};

