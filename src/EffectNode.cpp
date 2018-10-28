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
    : VideoNode(new EffectNodePrivate(context))
{
    attachSignals();
    d()->m_openGLWorker = QSharedPointer<EffectNodeOpenGLWorker>(new EffectNodeOpenGLWorker(*this), &QObject::deleteLater);
    setInputCount(1);
    setFrequency(0);
    d()->m_periodic.setInterval(10);
    d()->m_periodic.start();
    connect(&d()->m_periodic, &QTimer::timeout, this, &EffectNode::onPeriodic);

    d()->m_beatLast = context->timebase()->beat();
    d()->m_realTimeLast = context->timebase()->wallTime();
    if (!file.isEmpty()) setFile(file);
}

EffectNode::EffectNode(const EffectNode &other)
    : VideoNode(other)
{
    attachSignals();
}

EffectNode::EffectNode(QSharedPointer<EffectNodePrivate> other_ptr)
    : VideoNode(other_ptr.staticCast<VideoNodePrivate>())
{
    attachSignals();
}

EffectNode *EffectNode::clone() const {
    return new EffectNode(*this);
}

QSharedPointer<EffectNodePrivate> EffectNode::d() const {
    return d_ptr.staticCast<EffectNodePrivate>();
}

void EffectNode::attachSignals() {
    connect(d().data(), &EffectNodePrivate::intensityChanged, this, &EffectNode::intensityChanged);
    connect(d().data(), &EffectNodePrivate::nameChanged, this, &EffectNode::nameChanged);
    connect(d().data(), &EffectNodePrivate::fileChanged, this, &EffectNode::fileChanged);
    connect(d().data(), &EffectNodePrivate::frequencyChanged, this, &EffectNode::frequencyChanged);
}

QJsonObject EffectNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("file", d()->m_file);
    o.insert("intensity", d()->m_intensity);
    return o;
}

void EffectNode::chainsEdited(QList<Chain> added, QList<Chain> removed) {
    for (auto chain : added) {
        auto result = QMetaObject::invokeMethod(d()->m_openGLWorker.data(), "addNewState", Q_ARG(Chain, chain));
        Q_ASSERT(result);
    }
    {
        QMutexLocker locker(&d()->m_stateLock);
        for (int i=0; i<removed.count(); i++) {
            d()->m_renderStates.remove(removed.at(i));
        }
    }
}

GLuint EffectNode::paint(Chain chain, QVector<GLuint> inputTextures) {
    GLuint outTexture = 0;

    QSharedPointer<EffectNodeRenderState> renderState;
    int inputCount;
    auto time = context()->timebase()->beat();
    auto wallTime = context()->timebase()->wallTime();
    qreal step;
    qreal intensityIntegral;
    qreal frequency;
    {
        QMutexLocker locker(&d()->m_stateLock);
        if (!d()->m_ready) {
            //qDebug() << this << "is not ready";
            return inputTextures.at(0); // Pass-through
        }
        //qDebug() << "Looking up" << chain << "in" << d()->m_renderStates << "of" << this;
        if (!d()->m_renderStates.contains(chain)) {
            qDebug() << this << "does not have chain" << chain;
            return inputTextures.at(0);
        }
        renderState = d()->m_renderStates[chain];
        inputCount = d()->m_inputCount;
        d()->m_realTimeLast = d()->m_realTime;
        d()->m_realTime = wallTime;
        step = d()->m_realTime - d()->m_realTimeLast;
        intensityIntegral = d()->m_intensityIntegral;
        frequency = d()->m_frequency;
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
                pass.m_output = QSharedPointer<QOpenGLFramebufferObject>::create(chain.size(),fmt);
            }
        }
        if(!renderState->m_extra) {
            renderState->m_extra = QSharedPointer<QOpenGLFramebufferObject>::create(chain.size(),fmt);
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

        auto size = chain.size();
        glViewport(0, 0, size.width(), size.height());

        for(auto & pass : renderState->m_passes) {
            //qDebug() << "Rendering shader" << j << "onto" << (renderState->m_textureIndex + j + 1) % (d()->m_programs.count() + 1);
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
            glBindTexture(GL_TEXTURE_2D, chain.noiseTexture());
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
        //qDebug() << "Output is" << ((renderState->m_textureIndex + 1) % (d()->m_programs.count() + 1));
    }
    return outTexture;
}

void EffectNode::onPeriodic() {
    QMutexLocker locker(&d()->m_stateLock);
    qreal beatNow = context()->timebase()->beat();
    qreal beatDiff = beatNow - d()->m_beatLast;
    if (beatDiff < 0)
        beatDiff += Timebase::MAX_BEAT;
    d()->m_intensityIntegral = fmod(d()->m_intensityIntegral + d()->m_intensity * beatDiff, MAX_INTEGRAL);
    d()->m_beatLast = beatNow;
}

qreal EffectNode::intensity() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_intensity;
}

void EffectNode::setIntensity(qreal value) {
    {
        QMutexLocker locker(&d()->m_stateLock);
        if(value > 1) value = 1;
        if(value < 0) value = 0;
        if(d()->m_intensity == value)
            return;
        d()->m_intensity = value;
    }
    emit d()->intensityChanged(value);
}

QString EffectNode::file() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_file;
}

QString EffectNode::fileToName(QString file) {
    return QFileInfo(file).baseName();
}

QString EffectNode::name() {
    QMutexLocker locker(&d()->m_stateLock);
    return fileToName(d()->m_file);
}

double EffectNode::frequency() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_frequency;
}

void EffectNode::setFrequency(double frequency) {
    {
        QMutexLocker locker(&d()->m_stateLock);
        if (frequency == d()->m_frequency) return;
        d()->m_frequency = frequency;
    }
    emit d()->frequencyChanged(frequency);
}

void EffectNode::setFile(QString file) {
    file = Paths::contractLibraryPath(file);
    QString oldName;
    QString newName;
    {
        QMutexLocker locker(&d()->m_stateLock);
        oldName = fileToName(d()->m_file);
        if(file == d()->m_file) return;
        d()->m_file = file;
        newName = fileToName(d()->m_file);
    }
    reload();
    emit d()->fileChanged(file);
    if (newName != oldName) emit d()->nameChanged(newName);
}

void EffectNode::reload() {
    setNodeState(VideoNode::Loading);
    QString filename;
    {
        QMutexLocker locker(&d()->m_stateLock);
        d()->m_ready = false;
        filename = d()->m_file;
    }

    filename = Paths::expandLibraryPath(filename);

    QFileInfo check_file(filename);
    if(!(check_file.exists() && check_file.isFile())) {
        emit d()->error(QString("Could not open \"%1\"").arg(filename));
        setNodeState(VideoNode::Broken);
        return;
    }

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        emit d()->error(QString("Could not open \"%1\"").arg(filename));
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
        emit d()->error(QString("No shaders found for \"%1\"").arg(filename));
        setNodeState(VideoNode::Broken);
        return;
    }

    for (auto prop = props.keyBegin(); prop != props.keyEnd(); prop++) {
        auto value = props.value(*prop);
        setProperty(prop->toLatin1().data(), value);
    }

    bool result = QMetaObject::invokeMethod(d()->m_openGLWorker.data(), "initialize", Q_ARG(QVector<QStringList>, passes));
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

EffectNodeOpenGLWorker::EffectNodeOpenGLWorker(EffectNode p)
    : OpenGLWorker(p.context()->openGLWorkerContext())
    , m_p(p) {
    connect(this, &EffectNodeOpenGLWorker::message, p.d().data(), &EffectNodePrivate::message);
    connect(this, &EffectNodeOpenGLWorker::warning, p.d().data(), &EffectNodePrivate::warning);
    connect(this, &EffectNodeOpenGLWorker::error,   p.d().data(), &EffectNodePrivate::error);
}

void EffectNodeOpenGLWorker::initialize(QVector<QStringList> sourceCode) {
    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // EffectNode was deleted
    EffectNode p(d);

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
            p.setNodeState(VideoNode::Broken);
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
            p.setNodeState(VideoNode::Broken);
            return;
        }
        auto frag = headerString + "\n" + code.join("\n");
        if(!program->addShaderFromSourceCode(QOpenGLShader::Fragment, frag)) {
            auto log = program->log().trimmed();
            QRegularExpression re("0:(\\d+)\\((\\d+)\\):");
            log.replace(re, "<a href=\"editline,\\1,\\2\">\\1(\\2)</a>:");
            emit error(QString("Could not compile fragment shader:\n") + log);
            p.setNodeState(VideoNode::Broken);
            return;
        }
        if(!program->link()) {
            emit error("Could not link shader program");
            p.setNodeState(VideoNode::Broken);
            return;
        }
        shaders.append(program);
    }

    Q_ASSERT(!shaders.empty());
    std::reverse(shaders.begin(), shaders.end());
    // Shaders are now compiled and ready to go.

    // We prepare the state for all chains that exist upon creation
    auto chains = p.chains();
    QMap<Chain, QSharedPointer<EffectNodeRenderState>> states;

    for (auto chain : chains) {
        states.insert(chain, QSharedPointer<EffectNodeRenderState>::create(shaders));
    }

    // Swap out the newly loaded stuff
    {
        QMutexLocker locker(&p.d()->m_stateLock);
        p.d()->m_renderStates.clear();

        // Chains may have been deleted while we were preparing the states
        auto realChains = p.d()->m_chains;
        for (auto realChain: realChains) {
            if (states.contains(realChain)) {
                p.d()->m_renderStates.insert(realChain, states.value(realChain));
            }
        }

        p.d()->m_shaders = shaders;
        p.d()->m_ready = true;
    }

    glFlush();
    p.setNodeState(VideoNode::Ready);
}

// Invoke this method when a Chain gets added
// (or when the state for a given chain is somehow missing)
// It will create the new state asynchronously and add it when it is ready.
void EffectNodeOpenGLWorker::addNewState(Chain c) {
    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // EffectNode was deleted
    EffectNode p(d);

    QVector<QSharedPointer<QOpenGLShaderProgram>> shaders;
    {
        QMutexLocker locker(&p.d()->m_stateLock);
        // Don't make states that we don't have to
        if (!p.d()->m_ready) return;
        if (!p.d()->m_chains.contains(c)) return;
        if (p.d()->m_renderStates.contains(c)) return;
        shaders = p.d()->m_shaders;
    }

    makeCurrent();
    auto state = QSharedPointer<EffectNodeRenderState>::create(shaders);

    {
        QMutexLocker locker(&p.d()->m_stateLock);
        // Check that everything is still OK
        if (!p.d()->m_ready) return;
        if (!p.d()->m_chains.contains(c)) return;
        if (p.d()->m_renderStates.contains(c)) return;
        p.d()->m_renderStates.insert(c, state);
    }
}

QString EffectNode::typeName() {
    return "EffectNode";
}

VideoNode *EffectNode::deserialize(Context *context, QJsonObject obj) {
    QString file = obj.value("file").toString();
    if (obj.isEmpty()) {
        return nullptr;
    }
    EffectNode *e = new EffectNode(context, file);
    double intensity = obj.value("intensity").toDouble();
    e->setIntensity(intensity);
    return e;
}

bool EffectNode::canCreateFromFile(QString file) {
    return file.endsWith(".glsl", Qt::CaseInsensitive);
}

VideoNode *EffectNode::fromFile(Context *context, QString file) {
    EffectNode *e = new EffectNode(context, file);
    return e;
}

QMap<QString, QString> EffectNode::customInstantiators() {
    return QMap<QString, QString>();
}

WeakEffectNode::WeakEffectNode()
{
}

WeakEffectNode::WeakEffectNode(const EffectNode &other)
    : d_ptr(other.d())
{
}

QSharedPointer<EffectNodePrivate> WeakEffectNode::toStrongRef() {
    return d_ptr.toStrongRef();
}

EffectNodePrivate::EffectNodePrivate(Context *context)
    : VideoNodePrivate(context)
{
}
