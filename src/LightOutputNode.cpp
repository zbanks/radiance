#include "LightOutputNode.h"
#include "Context.h"

LightOutputNode::LightOutputNode(Context *context)
    : OutputNode(new LightOutputNodePrivate(context)) {
    attachSignals();
    d()->m_workerContext = new OpenGLWorkerContext(context->threaded());
    d()->m_worker = QSharedPointer<LightOutputNodeOpenGLWorker>(new LightOutputNodeOpenGLWorker(*this), &QObject::deleteLater);
    connect(d()->m_worker.data(), &QObject::destroyed, d()->m_workerContext, &QObject::deleteLater);

    d()->m_chain.moveToWorkerContext(d()->m_workerContext);

    {
        auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "initialize");
        Q_ASSERT(result);
    }
}

LightOutputNode::LightOutputNode(const LightOutputNode &other)
    : OutputNode(other)
{
    attachSignals();
}

LightOutputNode *LightOutputNode::clone() const {
    return new LightOutputNode(*this);
}

QSharedPointer<LightOutputNodePrivate> LightOutputNode::d() const {
    return d_ptr.staticCast<LightOutputNodePrivate>();
}

LightOutputNode::LightOutputNode(QSharedPointer<LightOutputNodePrivate> other_ptr)
    : OutputNode(other_ptr.staticCast<OutputNodePrivate>())
{
    attachSignals();
}

void LightOutputNode::attachSignals() {
    connect(d().data(), &LightOutputNodePrivate::urlChanged, this, &LightOutputNode::urlChanged);
}

QString LightOutputNode::typeName() {
    return "LightOutputNode";
}

VideoNode *LightOutputNode::deserialize(Context *context, QJsonObject obj) {
    LightOutputNode *e = new LightOutputNode(context);
    QString url = obj.value("url").toString();
    if (!url.isEmpty()) {
        e->setUrl(url);
    }
    return e;
}

QJsonObject LightOutputNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("url", url());
    return o;
}

bool LightOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *LightOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> LightOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("LightOutput", "LightOutputInstantiator.qml");
    return m;
}

QString LightOutputNode::url() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_url;
}

void LightOutputNode::setUrl(QString value) {
    {
        QMutexLocker locker(&d()->m_stateLock);
        if(d()->m_url == value)
            return;
        d()->m_url = value;
    }
    emit d()->urlChanged(value);
}

// WeakLightOutputNode methods

WeakLightOutputNode::WeakLightOutputNode()
{
}

WeakLightOutputNode::WeakLightOutputNode(const LightOutputNode &other)
    : d_ptr(other.d())
{
}

QSharedPointer<LightOutputNodePrivate> WeakLightOutputNode::toStrongRef() {
    return d_ptr.toStrongRef();
}

// LightOutputNodeOpenGLWorker methods

LightOutputNodeOpenGLWorker::LightOutputNodeOpenGLWorker(LightOutputNode p)
    : OpenGLWorker(p.d()->m_workerContext)
    , m_p(p) {
}

QSharedPointer<QOpenGLShaderProgram> LightOutputNodeOpenGLWorker::loadBlitShader() {
    Q_ASSERT(QThread::currentThread() == thread());
    auto vertexString = QString{
        "#version 150\n"
        "out vec2 uv;\n"
        "const vec2 varray[4] = vec2[](vec2(1., 1.),vec2(1., -1.),vec2(-1., 1.),vec2(-1., -1.));\n"
        "void main() {\n"
        "    vec2 vertex = varray[gl_VertexID];\n"
        "    gl_Position = vec4(vertex,0.,1.);\n"
        "    uv = 0.5*(vertex+1.);\n"
        "}\n"};
    auto fragmentString = QString{
        "#version 150\n"
        "uniform sampler2D iFrame;\n"
        "in vec2 uv;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    fragColor = texture(iFrame, uv);\n"
        "}\n"};

    auto shader = QSharedPointer<QOpenGLShaderProgram>(new QOpenGLShaderProgram());

    if (!shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexString)) {
        emit error("Could not compile vertex shader");
        return nullptr;
    }
    if (!shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentString)) {
        emit error("Could not compile fragment shader");
        return nullptr;
    }
    if (!shader->link()) {
        emit error("Could not link shader program");
        return nullptr;
    }

    return shader;
}

void LightOutputNodeOpenGLWorker::initialize() {
    Q_ASSERT(QThread::currentThread() == thread());

    QSize size(100, 100);

    makeCurrent();
    m_shader = loadBlitShader();
    if (m_shader.isNull()) return;

    auto fmt = QOpenGLFramebufferObjectFormat{};
    fmt.setInternalTextureFormat(GL_RGBA);
    m_fbo = QSharedPointer<QOpenGLFramebufferObject>::create(size, fmt);

    m_size = size;
    m_pixelBuffer.resize(4 * size.width() * size.height());

    //m_timer = new QTimer(this);
    //connect(m_timer, &QTimer::timeout, this, &LightOutputNodeOpenGLWorker::onTimeout);
}

void LightOutputNodeOpenGLWorker::render() {
    Q_ASSERT(QThread::currentThread() == thread());
    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // LightOutputNode was deleted
    LightOutputNode p(d);

    makeCurrent();
    GLuint texture = p.render();

    if (texture == 0) {
        qWarning() << "No frame available";
        return;
    }

    auto vao = p.chain().vao();

    glClearColor(0, 0, 0, 0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_fbo->bind();
    // VAO??
    glViewport(0, 0, m_size.width(), m_size.height());

    m_shader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    m_shader->setUniformValue("iFrame", 0);

    vao->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    vao->release();

    glReadPixels(0, 0, m_size.width(), m_size.height(), GL_RGBA, GL_UNSIGNED_BYTE, m_pixelBuffer.data());

    m_fbo->release();
    m_shader->release();
}

LightOutputNodePrivate::LightOutputNodePrivate(Context *context)
    : OutputNodePrivate(context, QSize(300, 300))
{
}
