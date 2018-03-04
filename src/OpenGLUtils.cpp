#include "OpenGLUtils.h"

QSharedPointer<QOpenGLShaderProgram> copyProgram(QSharedPointer<QOpenGLShaderProgram> program) {
    if(!program || !program->isLinked())
        return {};

    auto nprogram = QSharedPointer<QOpenGLShaderProgram>::create();
    for(auto shader : program->shaders())
        nprogram->addShader(shader);

    if(!nprogram->link())
        nprogram.reset();

    return nprogram;
}
