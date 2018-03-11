#include "FFmpegOutputNode.h"

FFmpegOutputNode::FFmpegOutputNode(Context *context, QSize chainSize, qreal fps)
    : OutputNode(new FFmpegOutputNodePrivate(context, chainSize)) {

    d()->m_workerContext = new OpenGLWorkerContext();
    d()->m_worker = QSharedPointer<FFmpegOpenGLWorker>(new FFmpegOpenGLWorker(*this), &QObject::deleteLater);
    connect(d()->m_worker.data(), &QObject::destroyed, d()->m_workerContext, &QObject::deleteLater);

    d()->m_chain.moveToWorkerContext(d()->m_workerContext);

    connect(d()->m_worker.data(), &FFmpegOpenGLWorker::initialized, this, &FFmpegOutputNode::initialize, Qt::DirectConnection);
    connect(d()->m_worker.data(), &FFmpegOpenGLWorker::frame, this, &FFmpegOutputNode::frame, Qt::DirectConnection);
    connect(this, &FFmpegOutputNode::fpsChanged, d()->m_worker.data(), &FFmpegOpenGLWorker::setFps);
    connect(this, &FFmpegOutputNode::ffmpegArgumentsChanged, d()->m_worker.data(), &FFmpegOpenGLWorker::setFFmpegArguments);

    {
        auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "initialize", Q_ARG(QSize, chainSize));
        Q_ASSERT(result);
    }
    setFps(fps);
    setFFmpegArguments({"-vcodec", "h264", "output.mp4"});
}

FFmpegOutputNode::FFmpegOutputNode(const FFmpegOutputNode &other)
    : OutputNode(other)
{
}

FFmpegOutputNode *FFmpegOutputNode::clone() const {
    return new FFmpegOutputNode(*this);
}

QSharedPointer<FFmpegOutputNodePrivate> FFmpegOutputNode::d() const {
    return d_ptr.staticCast<FFmpegOutputNodePrivate>();
}

FFmpegOutputNode::FFmpegOutputNode(QSharedPointer<FFmpegOutputNodePrivate> other_ptr)
    : OutputNode(other_ptr.staticCast<OutputNodePrivate>())
{
}

void FFmpegOutputNode::start() {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "start");
    Q_ASSERT(result);
    emit recordingChanged(true);
}

void FFmpegOutputNode::stop() {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "stop");
    Q_ASSERT(result);
    emit recordingChanged(false);
}

void FFmpegOutputNode::force() {
    auto result = QMetaObject::invokeMethod(d()->m_worker.data(), "renderFrame");
    Q_ASSERT(result);
}

void FFmpegOutputNode::setRecording(bool recording) {
    {
        QMutexLocker locker(&d()->m_stateLock);
        if (d()->m_recording == recording) 
            return;
    }

    if (recording)
        start();
    else
        stop();
}

bool FFmpegOutputNode::recording() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_recording;
}

QStringList FFmpegOutputNode::ffmpegArguments() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_ffmpegArguments;
}

void FFmpegOutputNode::setFFmpegArguments(QStringList ffmpegArguments) {
    setRecording(false);

    {
        QMutexLocker locker(&d()->m_stateLock);
        d()->m_ffmpegArguments = ffmpegArguments;
    }
    emit ffmpegArgumentsChanged(ffmpegArguments);
}

qreal FFmpegOutputNode::fps() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_fps;
}

void FFmpegOutputNode::setFps(qreal fps) {
    {
        QMutexLocker locker(&d()->m_stateLock);
        d()->m_fps = fps;
    }
    emit fpsChanged(fps);
}

QString FFmpegOutputNode::typeName() {
    return "FFmpegOutputNode";
}

VideoNode *FFmpegOutputNode::deserialize(Context *context, QJsonObject obj) {
    // TODO: You should be able to change the size of an OutputNode after
    // it has been created. For now this is hard-coded
    FFmpegOutputNode *e = new FFmpegOutputNode(context, QSize(128, 128));
    return e;
}

bool FFmpegOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *FFmpegOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> FFmpegOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("FFmpegOutput", "FFmpegOutputInstantiator.qml");
    return m;
}

// WeakSelcTimedReadBackOutputNode methods

WeakFFmpegOutputNode::WeakFFmpegOutputNode()
{
}

WeakFFmpegOutputNode::WeakFFmpegOutputNode(const FFmpegOutputNode &other)
    : d_ptr(other.d())
{
}

QSharedPointer<FFmpegOutputNodePrivate> WeakFFmpegOutputNode::toStrongRef() {
    return d_ptr.toStrongRef();
}

// FFmpegOpenGLWorker methods

FFmpegOpenGLWorker::FFmpegOpenGLWorker(FFmpegOutputNode p)
    : OpenGLWorker(p.d()->m_workerContext)
    , m_p(p)
    , m_initialized(false)
    , m_useTimer(true) {
}

QSharedPointer<QOpenGLShaderProgram> FFmpegOpenGLWorker::loadBlitShader() {
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
        emit fatal("Could not compile vertex shader");
        return nullptr;
    }
    if (!shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentString)) {
        emit fatal("Could not compile fragment shader");
        return nullptr;
    }
    if (!shader->link()) {
        emit fatal("Could not link shader program");
        return nullptr;
    }

    return shader;
}

void FFmpegOpenGLWorker::initialize(QSize size) {
    makeCurrent();
    m_shader = loadBlitShader();
    if (m_shader.isNull()) return;

    auto fmt = QOpenGLFramebufferObjectFormat{};
    fmt.setInternalTextureFormat(GL_RGBA);
    m_fbo = QSharedPointer<QOpenGLFramebufferObject>::create(size, fmt);

    m_size = size;
    m_pixelBuffer.resize(4 * size.width() * size.height());

    m_timer = QSharedPointer<QTimer>::create(this);
    connect(m_timer.data(), &QTimer::timeout, this, &FFmpegOpenGLWorker::renderFrame);

    m_ffmpeg = QSharedPointer<QProcess>::create(this);
    m_ffmpeg->setProgram("ffmpeg");
    qInfo() << "initialize";

    m_initialized = true;
    emit initialized();
}

void FFmpegOpenGLWorker::setFFmpegArguments(QStringList ffmpegArguments) {
    Q_ASSERT(QThread::currentThread() == thread());

    QString sizeStr(QString("%1x%2").arg(
                QString::number(m_size.width()),
                QString::number(m_size.height())));

    stop();
    m_ffmpeg->setArguments(QStringList() 
            << "-y"
            << "-vcodec" << "rawvideo"
            << "-f" << "rawvideo"
            << "-pix_fmt" << "rgba"
            << "-s" << sizeStr
            << "-i" << "pipe:0"
            << "-vf" << "vflip"
            << ffmpegArguments);
}

void FFmpegOpenGLWorker::setFps(qreal fps) {
    Q_ASSERT(QThread::currentThread() == thread());

    if (fps > 0) {
        m_timer->setInterval(1000. / fps);
        m_useTimer = true;
    } else {
        if (m_timer->isActive()) {
            m_timer->stop();
        }
        m_useTimer = false;
    }
}

void FFmpegOpenGLWorker::start() {
    if (!m_initialized) {
        qWarning() << "Node not ready, ignoring start";
        return;
    }
    Q_ASSERT(QThread::currentThread() == thread());

    m_ffmpeg->start();
    if (m_useTimer) {
        m_timer->start();
    }
}

void FFmpegOpenGLWorker::stop() {
    if (!m_initialized) {
        qWarning() << "Node not ready, ignoring start";
        return;
    }
    Q_ASSERT(QThread::currentThread() == thread());

    if (m_ffmpeg->state() != QProcess::NotRunning) {
        m_ffmpeg->closeWriteChannel();
        m_ffmpeg->waitForFinished();
    }
    if (m_useTimer) {
        m_timer->stop();
    }
}

void FFmpegOpenGLWorker::renderFrame() {
    if (!m_initialized) {
        qWarning() << "Node not ready, ignoring renderFrame";
        return;
    }
    Q_ASSERT(QThread::currentThread() == thread());


    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // FFmpegOutputNode was deleted
    FFmpegOutputNode p(d);

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

    if (m_ffmpeg->state() != QProcess::NotRunning) {
        m_ffmpeg->write(m_pixelBuffer);
        m_ffmpeg->waitForBytesWritten();
    }

    emit frame(m_size, m_pixelBuffer);
}

FFmpegOutputNodePrivate::FFmpegOutputNodePrivate(Context *context, QSize chainSize)
    : OutputNodePrivate(context, chainSize)
{
}
