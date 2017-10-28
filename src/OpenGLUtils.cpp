#include "OpenGLUtils.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
bool OpenGLSampler::create()
{
    destroy();
    auto ctx = QOpenGLContext::currentContext();
    if(!ctx)
        return false;
    auto funcs = ctx->extraFunctions();
    if(!funcs)
        return false;
    funcs->glGenSamplers(1, &m_id);
    if(!isCreated())
        return false;
    return true;
}
OpenGLSampler::~OpenGLSampler()
{
    destroy();
}
void OpenGLSampler::destroy()
{
    if(!isCreated())
        return;
    if(auto ctx = QOpenGLContext::currentContext()) {
        if(auto fns = ctx->extraFunctions())
            fns->glDeleteSamplers(1,&m_id);
            m_id = 0u;
    }
}
void OpenGLSampler::bind(GLuint unit)
{
    if(!isCreated())
        return;
    if(auto ctx = QOpenGLContext::currentContext()) {
        if(auto fns = ctx->extraFunctions())
            fns->glBindSampler(unit, samplerId());
    }
}
void OpenGLSampler::setParameter(GLenum pname, GLint param)
{
    if(!isCreated())
        return;
    if(auto ctx = QOpenGLContext::currentContext()) {
        if(auto fns = ctx->extraFunctions())
            fns->glSamplerParameteri(samplerId(), pname,param);
    }
}
void OpenGLSampler::setParameter(GLenum pname, GLfloat param)
{
    if(!isCreated())
        return;
    if(auto ctx = QOpenGLContext::currentContext()) {
        if(auto fns = ctx->extraFunctions())
            fns->glSamplerParameterf(samplerId(), pname, param);
    }
}
