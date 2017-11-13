#include "EffectNode.h"
#include "ProbeReg.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QRegularExpression>
#include <QOpenGLVertexArrayObject>
#include <memory>
#include <utility>
#include <functional>
#include <algorithm>
#include "Paths.h"
#include "main.h"

EffectType::EffectType(NodeRegistry *r , QObject *p )
    : NodeType(r,p) {
}
EffectType::~EffectType() = default;
VideoNode *EffectType::create(QString arg) {
    auto node = new EffectNode(this);
    if ( node ) {
        node->setInputCount(inputCount());
        node->setName(name());
        auto ok = false;
        auto intensity = arg.toFloat(&ok);

        if ( ok ) {
            node->setIntensity(intensity);
        }
    }
    return node;
}

EffectNode::EffectNode(NodeType *nr)
    : VideoNode(nr)
    , m_intensity(0)
    , m_openGLWorker(new EffectNodeOpenGLWorker(this), &QObject::deleteLater)
    , m_ready(false) {

    setInputCount(1);
    m_periodic.setInterval(10);
    m_periodic.start();
    connect(&m_periodic, &QTimer::timeout, this, &EffectNode::periodic);

    m_beatLast = timebase->beat();
    m_realTimeLast = timebase->wallTime();
    connect(m_openGLWorker.data(), &EffectNodeOpenGLWorker::initialized, this, &EffectNode::onInitialized);
}

EffectNode::EffectNode(const EffectNode &other)
    : VideoNode(other)
//    , m_programs(other.m_programs)
    , m_intensity(other.m_intensity)
    , m_intensityIntegral(other.m_intensityIntegral)
    , m_beatLast(other.m_beatLast)
    , m_realTime(other.m_realTime)
    , m_realTimeLast(other.m_realTimeLast)
    , m_openGLWorker(other.m_openGLWorker)
    , m_ready(other.m_ready) {
/*
    auto k = other.m_renderStates.keys();
    for (int i=0; i<k.count(); i++) {
        auto otherRenderState = other.m_renderStates.value(k.at(i));
        m_renderStates.insert(k.at(i), QSharedPointer<EffectNodeRenderState>(new EffectNodeRenderState(*otherRenderState)));
    }*/
}

EffectNode::~EffectNode() {
}

QString EffectNode::serialize() {
    return QString("%1:%2").arg(m_name, QString::number(m_intensity));
}

namespace {
std::once_flag reg_once{};
TypeRegistry effect_registry{[](NodeRegistry *r) -> QList<NodeType*> {
    std::call_once(reg_once,[](){
        qmlRegisterUncreatableType<EffectNode>("radiance",1,0,"EffectNode","EffectNode must be created through the registry");
    });
    auto res = QList<NodeType*>{};

    auto filters = QStringList{} << QString{"*.glsl"};
    QDir dir(Paths::library() + QString("effects/"));
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);

    for (auto effectName : dir.entryList()) {
        // TODO: Refactor the parsing logic so it's not duplicated
        auto name = effectName.replace(".glsl", "");
        auto filename = Paths::library() + QString("effects/%1.glsl").arg(name);

        QFileInfo check_file(filename);
        if(!(check_file.exists() && check_file.isFile()))
            continue;

        QFile file(filename);
        if(!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        QTextStream stream(&file);

        auto buffershader_reg = QRegularExpression(
            "^\\s*#buffershader\\s*$"
        , QRegularExpression::CaseInsensitiveOption
            );
        auto property_reg = QRegularExpression(
            "^\\s*#property\\s+(?<name>\\w+)\\s+(?<value>.*)$"
        , QRegularExpression::CaseInsensitiveOption
            );
        auto passes = QVector<QStringList>{QStringList{"#line 0"}};
        auto props  = QMap<QString,QString>{{"inputCount","1"}};
        auto lineno = 0;
        auto t = new EffectType(r);
        t->setName(name);
        t->setDescription(effectName);
        t->setInputCount(1);
        for(auto next_line = QString{}; stream.readLineInto(&next_line);++lineno) {
            {
                auto m = property_reg.match(next_line);
                if(m.hasMatch()) {
                    props.insert(m.captured("name"),m.captured("value"));
                    passes.back().append(QString{"#line %1"}.arg(lineno));
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
        t->setInputCount(QVariant::fromValue(props["inputCount"]).value<int>());
        t->setDescription(QVariant::fromValue(props["description"]).value<QString>());
        t->setAuthor(QVariant::fromValue(props["author"]).value<QString>());

        res.append(t);
    }
    return res;
}};
}

void EffectNode::onInitialized() {
    {
        QMutexLocker locker(&m_stateLock);
        for (auto key : m_renderStates.keys()) {
            auto state = QSharedPointer<EffectNodeRenderState>::create();
            m_openGLWorker->prepareState(state);
            m_renderStates[key].swap(state);
        }
    }
    m_ready = true;
}

void EffectNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
    QMutexLocker locker(&m_stateLock);
    for (int i=0; i<added.count(); i++) {
        auto state = QSharedPointer<EffectNodeRenderState>::create();
        m_openGLWorker->prepareState(state);
        m_renderStates.insert(added.at(i), state);
    }
    for (int i=0; i<removed.count(); i++) {
        m_renderStates.remove(removed.at(i));
    }
}

// Paint never needs to take the stateLock
// because it already has a copy of the object
// and this copy will never be modified
// out from under it, unlike the parent
// from which it was created
GLuint EffectNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
    // Hitting this assert means
    // that you failed to make a copy
    // of the VideoNode
    // before rendering in a different thread
    Q_ASSERT(QThread::currentThread() == thread());

    GLuint outTexture = 0;

    if (!m_ready) {
        //qDebug() << this << "is not ready";
        return inputTextures.at(0); // Pass-through
    }
    if (!m_renderStates.contains(chain)) {
        qDebug() << this << "does not have chain" << chain;
        return inputTextures.at(0);
    }
    auto renderState = m_renderStates[chain];

    if(!renderState->ready())
        return inputTextures.at(0);
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
        auto chanTex = std::make_unique<GLuint[]>(renderState->size());
        std::iota(&chanTex[0], &chanTex[0] + renderState->size(), 1 + m_inputCount);
        std::reverse(&chanTex[0],&chanTex[0] + renderState->size());
        auto inputTex = std::make_unique<GLuint[]>(m_inputCount);
        std::iota(&inputTex[0], &inputTex[0] + m_inputCount, 0);
        auto   time = timebase->beat();
        m_realTimeLast = m_realTime;
        m_realTime     = timebase->wallTime();
        auto   step = m_realTime - m_realTimeLast;
        double audioHi = 0;
        double audioMid = 0;
        double audioLow = 0;
        double audioLevel = 0;
        audio->levels(&audioHi, &audioMid, &audioLow, &audioLevel);

        auto size = chain->size();
        glViewport(0, 0, size.width(), size.height());

        for(auto & pass : renderState->m_passes) {
            //qDebug() << "Rendering shader" << j << "onto" << (renderState->m_textureIndex + j + 1) % (m_programs.count() + 1);
            auto && p = pass.m_shader;
            renderState->m_extra->bind();
            p->bind();

            auto texCount = GL_TEXTURE0;
            for (int k = 0; k < m_inputCount; k++) {
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
            p->setUniformValue("iIntensityIntegral", GLfloat(m_intensityIntegral));
            p->setUniformValue("iStep", GLfloat(step));
            p->setUniformValue("iTime", GLfloat(time));
            p->setUniformValue("iFPS",  GLfloat(FPS));
            p->setUniformValue("iAudio", QVector4D(GLfloat(audioLow),GLfloat(audioMid),GLfloat(audioHi),GLfloat(audioLevel)));
            p->setUniformValueArray("iInputs", &inputTex[0], m_inputCount);
            p->setUniformValue("iNoise", m_inputCount);
            p->setUniformValue("iResolution", GLfloat(size.width()), GLfloat(size.height()));
            p->setUniformValueArray("iChannel", &chanTex[0], renderState->size());

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

void EffectNode::periodic() {
    Q_ASSERT(QThread::currentThread() == thread());

    QMutexLocker locker(&m_stateLock);
    qreal beatNow = timebase->beat();
    qreal beatDiff = beatNow - m_beatLast;
    if (beatDiff < 0)
        beatDiff += Timebase::MAX_BEAT;
    m_intensityIntegral = fmod(m_intensityIntegral + m_intensity * beatDiff, MAX_INTEGRAL);
    m_beatLast = beatNow;
}

qreal EffectNode::intensity() {
    Q_ASSERT(QThread::currentThread() == thread());
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

QString EffectNode::name() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_name;
}

void EffectNode::setName(QString name) {
    Q_ASSERT(QThread::currentThread() == thread());
    if(name != m_name) {
        m_ready = false;
        {
            QMutexLocker locker(&m_stateLock);
            m_name = name;
        }
        bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "initialize");
        Q_ASSERT(result);
        emit nameChanged(name);
    }
}

void EffectNode::reload() {
    m_ready = false;
    bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "initialize");
    Q_ASSERT(result);
}

// Creates a copy of this node for rendering
QSharedPointer<VideoNode> EffectNode::createCopyForRendering(QSharedPointer<Chain> chain) {
    //periodic();
    {
        QMutexLocker locker(&m_stateLock);
        auto v = QSharedPointer<EffectNode>::create(*this);
        auto tmpStates = decltype(m_renderStates){};
        tmpStates.insert(chain,m_renderStates.value(chain));
        v->m_renderStates.swap(tmpStates);
        // it's in the creating thread, since no owner.
        return v;
    }
}

// EffectNodeOpenGLWorker methods

EffectNodeOpenGLWorker::EffectNodeOpenGLWorker(EffectNode *p)
    : OpenGLWorker(p->workerContext())
    , m_p(p) {
    qRegisterMetaType<QSharedPointer<EffectNodeRenderState>>();
    connect(this, &EffectNodeOpenGLWorker::prepareState, this, &EffectNodeOpenGLWorker::onPrepareState);
    connect(this, &EffectNodeOpenGLWorker::message, m_p, &EffectNode::message);
    connect(this, &EffectNodeOpenGLWorker::warning, m_p, &EffectNode::warning);
    connect(this, &EffectNodeOpenGLWorker::fatal,   m_p, &EffectNode::fatal);
}

void EffectNodeOpenGLWorker::initialize() {
    makeCurrent();
    bool result = loadProgram(m_p->m_name);
    if(!result) {
        qDebug() << m_p << "Load program failed :(";
        return;
    }
    glFlush();
    emit initialized();
}
void EffectNodeOpenGLWorker::onPrepareState(QSharedPointer<EffectNodeRenderState> state) {
    if(!state || !m_state || state->m_ready.load())
        return;
    for(auto && pass : m_state->m_passes) {
        auto dprogram = QSharedPointer<QOpenGLShaderProgram>::create();
        for(auto && shader : pass.m_shader->shaders())
            dprogram->addShader(shader);
        if(!dprogram->link())
            return;
        state->m_passes.emplace_back();
        state->m_passes.back().m_shader =  dprogram;
    }
    state->m_ready.exchange(true);
}

// Call this to load shader code into this Effect.
// Returns true if the program was loaded successfully
bool EffectNodeOpenGLWorker::loadProgram(QString name) {
    Q_ASSERT(!m_p->m_ready); // Must have been marked unready

    auto headerString = QString{};
    {
        auto effectHeaderFilename = Paths::glsl() + QString("effect_header.glsl");
        QFile header_file(effectHeaderFilename);
        if(!header_file.open(QIODevice::ReadOnly)) {
            emit fatal(QString("Could not open \"%1\"").arg(effectHeaderFilename));
            return false;
        }
        QTextStream headerStream(&header_file);
        headerString = headerStream.readAll();
    }
    auto vertexString = QString{
        "#version 150\n"
        "const vec2 varray[4] = vec2[](vec2(1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.));\n"
        "out vec2 uv;\n"
        "void main() {"
        "    vec2 vertex = varray[gl_VertexID];\n"
        "    gl_Position = vec4(vertex,0.,1.);\n"
        "    uv = 0.5 * (vertex + 1.);\n"
        "}"};
    auto filename = Paths::library() + QString("effects/%1.glsl").arg(name);

    QFileInfo check_file(filename);
    if(!(check_file.exists() && check_file.isFile())) {
        emit fatal(QString("Could not open \"%1\"").arg(filename));
        return false;
    }

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        emit fatal(QString("Could not open \"%1\"").arg(filename));
        return false;
    }

    QTextStream stream(&file);

    auto buffershader_reg = QRegularExpression(
        "^\\s*#buffershader\\s*$"
      , QRegularExpression::CaseInsensitiveOption
        );
    auto property_reg = QRegularExpression(
        "^\\s*#property\\s+(?<name>\\w+)\\s+(?<value>.*)$"
      , QRegularExpression::CaseInsensitiveOption
        );
    auto state = QSharedPointer<EffectNodeRenderState>::create();
    auto &programs = state->m_passes;

    auto passes = QVector<QStringList>{QStringList{"#line 0"}};
    auto props  = QMap<QString,QString>{{"inputCount","1"}};
    auto lineno = 1;
    for(auto next_line = QString{}; stream.readLineInto(&next_line);++lineno) {
        {
            auto m = property_reg.match(next_line);
            if(m.hasMatch()) {
                props.insert(m.captured("name"),m.captured("value"));
                passes.back().append(QString{"#line %1"}.arg(lineno));
                //qDebug() << "setting property " << m.captured("name") << " to value " << m.captured("value");
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

    for(auto pass : passes) {
        programs.emplace_back();
        programs.back().m_shader = QSharedPointer<QOpenGLShaderProgram>::create();
        auto && program = programs.back().m_shader;
        if(!program->addShaderFromSourceCode(
            QOpenGLShader::Vertex
          , vertexString
            )) {
            emit fatal("Could not compile vertex shader");
            return false;
        }
        auto frag = headerString + "\n" + pass.join("\n");
        if(!program->addShaderFromSourceCode(QOpenGLShader::Fragment, frag)) {
            emit fatal(QString("Could not compile fragment shader:\n") + program->log().trimmed());
            return false;
        }
        if(!program->link()) {
            emit fatal("Could not link shader program");
            return false;
        }
    }
    if(programs.empty()) {
        emit fatal(QString("No shaders found for \"%1\"").arg(name));
        return false;
    }
    std::reverse(state->m_passes.begin(),state->m_passes.end());
    state->m_ready.exchange(true);
    m_state = state;
//    {
//        QMutexLocker locker(&m_p->m_stateLock);
//    }
    return true;
}

