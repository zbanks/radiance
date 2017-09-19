#include "ImageNode.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include "main.h"

static bool ilInitted = false;

ImageNode::ImageNode()
    : m_openGLWorker(new ImageNodeOpenGLWorker(this))
    , m_ready(false) {

    m_periodic.setInterval(100);
    m_periodic.start();
    connect(&m_periodic, &QTimer::timeout, this, &ImageNode::periodic);
    connect(m_openGLWorker.data(), &ImageNodeOpenGLWorker::initialized, this, &ImageNode::onInitialized);
}

ImageNode::ImageNode(const ImageNode &other)
    : VideoNode(other)
    , m_openGLWorker(other.m_openGLWorker)
    , m_frameTextures(other.m_frameTextures)
    , m_currentTexture(other.m_currentTexture)
    , m_currentTextureIdx(other.m_currentTextureIdx)
    , m_imagePath(other.m_imagePath)
    , m_ready(other.m_ready) {
}

ImageNode::~ImageNode() {
}

void ImageNode::onInitialized() {
    m_ready = true;
}

void imagePathChanged(QString imagePath) {
}

void ImageNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
}

void ImageNode::periodic() {
    Q_ASSERT(QThread::currentThread() == thread());

    // Lock this because we need to use m_frameTextures
    QMutexLocker locker(&m_stateLock);

    // TODO: actually use m_frameDelays
    m_currentTextureIdx = (++m_currentTextureIdx) % m_frameTextures.size();
    m_currentTexture = m_frameTextures.at(m_currentTextureIdx);
}

QString ImageNode::imagePath() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_imagePath;
}

void ImageNode::setImagePath(QString imagePath) {
    Q_ASSERT(QThread::currentThread() == thread());
    if(imagePath != m_imagePath) {
        m_ready = false;
        {
            QMutexLocker locker(&m_stateLock);
            m_imagePath = imagePath;
        }
        bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "initialize");
        Q_ASSERT(result);
        emit imagePathChanged(imagePath);
    }
}

// See comments in ImageNode.h about these 3 functions
QSharedPointer<VideoNode> ImageNode::createCopyForRendering() {
    return QSharedPointer<VideoNode>(new ImageNode(*this));
}

GLuint ImageNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
    if (!m_ready || !m_currentTexture) return 0;
    return m_currentTexture->textureId();
}

void ImageNode::copyBackRenderState(QSharedPointer<Chain> chain, QSharedPointer<VideoNode> copy) {
}

// ImageNodeOpenGLWorker methods

ImageNodeOpenGLWorker::ImageNodeOpenGLWorker(ImageNode *p)
    : OpenGLWorker(openGLWorkerContext)
    , m_p(p) {
    connect(this, &ImageNodeOpenGLWorker::message, m_p, &ImageNode::message);
    connect(this, &ImageNodeOpenGLWorker::warning, m_p, &ImageNode::warning);
    connect(this, &ImageNodeOpenGLWorker::fatal,   m_p, &ImageNode::fatal);
}

void ImageNodeOpenGLWorker::initialize() {
    // Lock this because we need to use m_frameTextures
    QMutexLocker locker(&m_p->m_stateLock);

    makeCurrent();
    bool result = loadImage(m_p->m_imagePath);
    if (!result) {
        qWarning() << "Can't load image!" << m_p->m_imagePath;
        return;
    }

    // Set the current frame to 0
    m_p->m_currentTextureIdx = 0;
    m_p->m_currentTexture = m_p->m_frameTextures.at(0);
    emit initialized();
}

// Call this to load an image into m_frameTextures
// Returns true if the program was loaded successfully
bool ImageNodeOpenGLWorker::loadImage(QString imagePath) {
    QString filename = QString("../resources/images/%1").arg(imagePath);
    QFile file(filename);

    QFileInfo check_file(filename);
    if(!(check_file.exists() && check_file.isFile())) {
        qWarning() << "Could not find" << filename;
        return false;
    }

    QImageReader imageReader(filename);
    int nFrames = imageReader.imageCount();
    if (nFrames == 0)
        nFrames = 1; // Returns 0 if animation isn't supported

    m_p->m_frameTextures.resize(nFrames);
    m_p->m_frameTextures.squeeze();
    m_p->m_frameDelays.resize(nFrames);
    m_p->m_frameDelays.squeeze();

    for (int i = 0; i < nFrames; i++) {
        QImage frame = imageReader.read();
        if (frame.isNull()) {
            qWarning() << "Unable to read frame" << i << "of image:" << imageReader.errorString();
            return false;
        }

        m_p->m_frameDelays[i] = imageReader.nextImageDelay();
        m_p->m_frameTextures[i] = QSharedPointer<QOpenGLTexture>(
                new QOpenGLTexture(frame.mirrored(), QOpenGLTexture::MipMapGeneration::GenerateMipMaps));
    }

    glFlush();
    qDebug() << "Successfully loaded image" << filename << "with" << nFrames << "frames";

    return true;
}
