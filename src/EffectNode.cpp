#include "EffectNode.h"
#include "Timebase.h"
#include "Audio.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QRegularExpression>
#include <QOpenGLVertexArrayObject>
#include <QtQml>
#include <memory>
#include <utility>
#include <functional>
#include <algorithm>
#include "Paths.h"

EffectNode::EffectNode(Context *context, QString file)
    : VideoNode(context)
{
    attachSignals();
    m_openGLWorker = QSharedPointer<EffectNodeOpenGLWorker>(new EffectNodeOpenGLWorker(qSharedPointerCast<EffectNode>(sharedFromThis())), &QObject::deleteLater);
    setInputCount(1);
    setFrequency(0);
    m_periodic.setInterval(10);
    m_periodic.start();
    connect(&m_periodic, &QTimer::timeout, this, &EffectNode::onPeriodic);

    m_beatLast = context->timebase()->beat();
    m_realTimeLast = context->timebase()->wallTime();
    if (!file.isEmpty()) setFile(file);
}

QJsonObject EffectNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("file", file());
    o.insert("intensity", intensity());
    return o;
}

void EffectNode::chainsEdited(QList<ChainSP> added, QList<ChainSP> removed) {
    for (auto chain : added) {
        auto result = QMetaObject::invokeMethod(m_openGLWorker.data(), "addNewState", Q_ARG(ChainSP, chain));
        Q_ASSERT(result);
    }
    {
        QMutexLocker locker(&m_stateLock);
        for (int i=0; i<removed.count(); i++) {
            m_renderStates.remove(removed.at(i));
        }
    }
}

GLuint EffectNode::paint(ChainSP chain, QVector<GLuint> inputTextures) {
    GLuint outTexture = 0;

    QSharedPointer<EffectNodeRenderState> renderState;
    int inputCount;
    auto time = context()->timebase()->beat();
    auto wallTime = context()->timebase()->wallTime();
    qreal step;
    qreal intensityIntegral;
    qreal frequency;
    {
        QMutexLocker locker(&m_stateLock);
        if (!m_ready) {
            //qDebug() << this << "is not ready";
            return inputTextures.at(0); // Pass-through
        }
        //qDebug() << "Looking up" << chain << "in" << m_renderStates << "of" << this;
        if (!m_renderStates.contains(chain)) {
            qDebug() << this << "does not have chain" << chain;
            return inputTextures.at(0);
        }
        renderState = m_renderStates[chain];
        inputCount = m_inputCount;
        m_realTimeLast = m_realTime;
        m_realTime = wallTime;
        step = m_realTime - m_realTimeLast;
        intensityIntegral = m_intensityIntegral;
        frequency = m_frequency;
    }

    // FBO creation must happen here, and not in initialize,
    // because FBOs are not shared among contexts.
    // Textures are, however, so in the future maybe we can move
    // texture creation to initialize()
    // and leave lightweight FBO creation here
    {
        auto fmt = QOpenGLFramebufferObjectFormat{};
        fmt.setInternalTextureFormat(GL_RGBA);
        for(auto & pass : renderState->m_passes) {
            if(!pass.m_output) {
                pass.m_output = QSharedPointer<QOpenGLFramebufferObject>::create(chain->size(),fmt);
            }
        }
        if(!renderState->m_extra) {
            renderState->m_extra = QSharedPointer<QOpenGLFramebufferObject>::create(chain->size(),fmt);
        }
    }

    glClearColor(0, 0, 0, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    {
        auto chanTex = std::make_unique<GLuint[]>(renderState->m_passes.size());
        std::iota(&chanTex[0], &chanTex[0] + renderState->m_passes.size(), 1 + inputCount);
        std::reverse(&chanTex[0],&chanTex[0] + renderState->m_passes.size());
        auto inputTex = std::make_unique<GLuint[]>(inputCount);
        std::iota(&inputTex[0], &inputTex[0] + inputCount, 0);
        double audioHi = 0;
        double audioMid = 0;
        double audioLow = 0;
        double audioLevel = 0;
        context()->audio()->levels(&audioHi, &audioMid, &audioLow, &audioLevel);

        auto size = chain->size();
        glViewport(0, 0, size.width(), size.height());

        for(auto & pass : renderState->m_passes) {
            //qDebug() << "Rendering shader" << j << "onto" << (renderState->m_textureIndex + j + 1) % (m_programs.count() + 1);
            auto && p = pass.m_shader;
            renderState->m_extra->bind();
            p->bind();

            auto texCount = GL_TEXTURE0;
            for (int k = 0; k < inputCount; k++) {
                glActiveTexture(texCount++);
                glBindTexture(GL_TEXTURE_2D, inputTextures.at(k));
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

            }

            glActiveTexture(texCount++);
            glBindTexture(GL_TEXTURE_2D, chain->noiseTexture());
            for(auto && op : renderState->m_passes) {
                glActiveTexture(texCount++);
                glBindTexture(GL_TEXTURE_2D, op.m_output->texture());
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            }
            auto intense = qreal(intensity());
            p->setUniformValue("iIntensity", GLfloat(intense));
            p->setUniformValue("iIntensityIntegral", GLfloat(intensityIntegral));
            p->setUniformValue("iStep", GLfloat(step));
            p->setUniformValue("iTime", GLfloat(time));
            p->setUniformValue("iFrequency", GLfloat(frequency));
            p->setUniformValue("iFPS",  GLfloat(FPS));
            p->setUniformValue("iAudio", QVector4D(GLfloat(audioLow),GLfloat(audioMid),GLfloat(audioHi),GLfloat(audioLevel)));
            p->setUniformValueArray("iInputs", &inputTex[0], inputCount);
            p->setUniformValue("iNoise", inputCount);
            p->setUniformValue("iResolution", GLfloat(size.width()), GLfloat(size.height()));
            p->setUniformValueArray("iChannel", &chanTex[0], renderState->m_passes.size());

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            renderState->m_extra->release();
            outTexture = renderState->m_extra->texture();
            using std::swap;
            swap(renderState->m_extra, pass.m_output);
            //renderState->m_intermediate.at(fboIndex)->toImage().save(QString("out_%1.png").arg(renderState->m_intermediate.at(fboIndex)->texture()));
            p->release();
            glActiveTexture(GL_TEXTURE0); // Very important to reset OpenGL state for scene graph rendering
        }
        //qDebug() << this << "Output texture ID is" << outTexture << renderState;
        //qDebug() << "Output is" << ((renderState->m_textureIndex + 1) % (m_programs.count() + 1));
    }
    return outTexture;
}

void EffectNode::onPeriodic() {
    QMutexLocker locker(&m_stateLock);
    qreal beatNow = context()->timebase()->beat();
    qreal beatDiff = beatNow - m_beatLast;
    if (beatDiff < 0)
        beatDiff += Timebase::MAX_BEAT;
    m_intensityIntegral = fmod(m_intensityIntegral + m_intensity * beatDiff, MAX_INTEGRAL);
    m_beatLast = beatNow;
}

qreal EffectNode::intensity() {
    QMutexLocker locker(&m_stateLock);
    return m_intensity;
}

void EffectNode::setIntensity(qreal value) {
    {
        QMutexLocker locker(&m_stateLock);
        if(value > 1) value = 1;
        if(value < 0) value = 0;
        if(m_intensity == value)
            return;
        m_intensity = value;
    }
    emit intensityChanged(value);
}

QString EffectNode::file() {
    QMutexLocker locker(&m_stateLock);
    return m_file;
}

QString EffectNode::fileToName(QString file) {
    return QFileInfo(file).baseName();
}

QString EffectNode::name() {
    QMutexLocker locker(&m_stateLock);
    return fileToName(m_file);
}

double EffectNode::frequency() {
    QMutexLocker locker(&m_stateLock);
    return m_frequency;
}

void EffectNode::setFrequency(double frequency) {
    {
        QMutexLocker locker(&m_stateLock);
        if (frequency == m_frequency) return;
        m_frequency = frequency;
    }
    emit frequencyChanged(frequency);
}

void EffectNode::setFile(QString file) {
    file = Paths::contractLibraryPath(file);
    QString oldName;
    QString newName;
    {
        QMutexLocker locker(&m_stateLock);
        oldName = fileToName(m_file);
        if(file == m_file) return;
        m_file = file;
        newName = fileToName(m_file);
    }
    reload();
    emit fileChanged(file);
    if (newName != oldName) emit nameChanged(newName);
}

void EffectNode::reload() {
    setNodeState(VideoNode::Loading);
    QString filename;
    {
        QMutexLocker locker(&m_stateLock);
        m_ready = false;
        filename = m_file;
    }

    filename = Paths::expandLibraryPath(filename);

    QFileInfo check_file(filename);
    if(!(check_file.exists() && check_file.isFile())) {
        emit error(QString("Could not open \"%1\"").arg(filename));
        setNodeState(VideoNode::Broken);
        return;
    }

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        emit error(QString("Could not open \"%1\"").arg(filename));
        setNodeState(VideoNode::Broken);
        return;
    }

    QTextStream stream(&file);

    auto buffershader_reg = QRegularExpression(
        "^\\s*#buffershader\\s*$"
      , QRegularExpression::CaseInsensitiveOption
        );
    auto property_reg = QRegularExpression(
        "^\\s*#property\\s+(?<file>\\w+)\\s+(?<value>.*)$"
      , QRegularExpression::CaseInsensitiveOption
        );

    auto passes = QVector<QStringList>{QStringList{"#line 0"}};
    auto props  = QMap<QString,QString>{{"inputCount","1"}};
    auto lineno = 1;
    for(auto next_line = QString{}; stream.readLineInto(&next_line);++lineno) {
        {
            auto m = property_reg.match(next_line);
            if(m.hasMatch()) {
                props.insert(m.captured("file"),m.captured("value"));
                passes.back().append(QString{"#line %1"}.arg(lineno));
                //qDebug() << "setting property " << m.captured("file") << " to value " << m.captured("value");
                continue;
            }
        }
        {
            auto m = buffershader_reg.match(next_line);
            if(m.hasMatch()) {
                passes.append({QString{"#line %1"}.arg(lineno)});
                continue;
            }
        }
        passes.back().append(next_line);
    }

    if(passes.empty()) {
        emit error(QString("No shaders found for \"%1\"").arg(filename));
        setNodeState(VideoNode::Broken);
        return;
    }

    for (auto prop = props.keyBegin(); prop != props.keyEnd(); prop++) {
        auto value = props.value(*prop);
        setProperty(prop->toLatin1().data(), value);
    }

    bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "initialize", Q_ARG(QVector<QStringList>, passes));
    Q_ASSERT(result);
}

// EffectNodeRenderState methods

// Requires a valid OpenGL context
EffectNodeRenderState::EffectNodeRenderState(QVector<QSharedPointer<QOpenGLShaderProgram>> shaders) {
    for(auto shader : shaders) {
        Pass p;
        p.m_shader = copyProgram(shader);
        m_passes.append(p);
    }
}

// EffectNodeOpenGLWorker methods

EffectNodeOpenGLWorker::EffectNodeOpenGLWorker(QSharedPointer<EffectNode> p)
    : OpenGLWorker(p->context()->openGLWorkerContext())
    , m_p(p) {
    connect(this, &EffectNodeOpenGLWorker::message, p.data(), &EffectNode::message);
    connect(this, &EffectNodeOpenGLWorker::warning, p.data(), &EffectNode::warning);
    connect(this, &EffectNodeOpenGLWorker::error,   p.data(), &EffectNode::error);
}

void EffectNodeOpenGLWorker::initialize(QVector<QStringList> sourceCode) {
    auto p = m_p.toStrongRef();
    if (p.isNull()) return; // EffectNode was deleted

    makeCurrent();
 
    auto vertexString = QString{
        "#version 150\n"
        "const vec2 varray[4] = vec2[](vec2(1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.));\n"
        "out vec2 uv;\n"
        "void main() {"
        "    vec2 vertex = varray[gl_VertexID];\n"
        "    gl_Position = vec4(vertex,0.,1.);\n"
        "    uv = 0.5 * (vertex + 1.);\n"
        "}"};

    auto headerString = QString{};
    {
        auto effectHeaderFilefile = Paths::glsl() + "/effect_header.glsl";
        QFile header_file(effectHeaderFilefile);
        if(!header_file.open(QIODevice::ReadOnly)) {
            emit error(QString("Could not open \"%1\"").arg(effectHeaderFilefile));
            p->setNodeState(VideoNode::Broken);
            return;
        }
        QTextStream headerStream(&header_file);
        headerString = headerStream.readAll();
    }

    QVector<QSharedPointer<QOpenGLShaderProgram>> shaders;

    for(auto code : sourceCode) {
        auto program = QSharedPointer<QOpenGLShaderProgram>::create();
        if(!program->addShaderFromSourceCode(
            QOpenGLShader::Vertex
          , vertexString
            )) {
            emit error("Could not compile vertex shader");
            p->setNodeState(VideoNode::Broken);
            return;
        }
        auto frag = headerString + "\n" + code.join("\n");
        if(!program->addShaderFromSourceCode(QOpenGLShader::Fragment, frag)) {
            auto log = program->log().trimmed();
            QRegularExpression re("0:(\\d+)\\((\\d+)\\):");
            log.replace(re, "<a href=\"editline,\\1,\\2\">\\1(\\2)</a>:");
            emit error(QString("Could not compile fragment shader:\n") + log);
            p->setNodeState(VideoNode::Broken);
            return;
        }
        if(!program->link()) {
            emit error("Could not link shader program");
            p->setNodeState(VideoNode::Broken);
            return;
        }
        shaders.append(program);
    }

    Q_ASSERT(!shaders.empty());
    std::reverse(shaders.begin(), shaders.end());
    // Shaders are now compiled and ready to go.

    // We prepare the state for all chains that exist upon creation
    auto chains = p->chains();
    QMap<ChainSP, QSharedPointer<EffectNodeRenderState>> states;

    for (auto chain : chains) {
        states.insert(chain, QSharedPointer<EffectNodeRenderState>::create(shaders));
    }

    // Swap out the newly loaded stuff
    {
        QMutexLocker locker(&p->m_stateLock);
        p->m_renderStates.clear();

        // Chains may have been deleted while we were preparing the states
        auto realChains = p->m_chains;
        for (auto realChain: realChains) {
            if (states.contains(realChain)) {
                p->m_renderStates.insert(realChain, states.value(realChain));
            }
        }

        p->m_shaders = shaders;
        p->m_ready = true;
    }

    glFlush();
    p->setNodeState(VideoNode::Ready);
}

// Invoke this method when a ChainSP gets added
// (or when the state for a given chain is somehow missing)
// It will create the new state asynchronously and add it when it is ready.
void EffectNodeOpenGLWorker::addNewState(ChainSP c) {
    auto p = m_p.toStrongRef();
    if (p.isNull()) return; // EffectNode was deleted

    QVector<QSharedPointer<QOpenGLShaderProgram>> shaders;
    {
        QMutexLocker locker(&p->m_stateLock);
        // Don't make states that we don't have to
        if (!p->m_ready) return;
        if (!p->m_chains.contains(c)) return;
        if (p->m_renderStates.contains(c)) return;
        shaders = p->m_shaders;
    }

    makeCurrent();
    auto state = QSharedPointer<EffectNodeRenderState>::create(shaders);

    {
        QMutexLocker locker(&p->m_stateLock);
        // Check that everything is still OK
        if (!p->m_ready) return;
        if (!p->m_chains.contains(c)) return;
        if (p->m_renderStates.contains(c)) return;
        p->m_renderStates.insert(c, state);
    }
}

QString EffectNode::typeName() {
    return "EffectNode";
}

VideoNodeSP *EffectNode::deserialize(Context *context, QJsonObject obj) {
    QString file = obj.value("file").toString();
    if (obj.isEmpty()) {
        return nullptr;
    }
    auto e = new EffectNode(context, file);
    double intensity = obj.value("intensity").toDouble();
    e->setIntensity(intensity);
    return new VideoNodeSP((VideoNode*)e);
}

bool EffectNode::canCreateFromFile(QString file) {
    return file.endsWith(".glsl", Qt::CaseInsensitive);
}

VideoNodeSP *EffectNode::fromFile(Context *context, QString file) {
    return new VideoNodeSP(new EffectNode(context, file));
}

QMap<QString, QString> EffectNode::customInstantiators() {
    return QMap<QString, QString>();
}
