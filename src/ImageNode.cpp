#include "ImageNode.h"
#include "ProbeReg.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include "Paths.h"
#include "main.h"


ImageType::ImageType(NodeRegistry *r , QObject *p )
    : NodeType(r,p) {
}
ImageType::~ImageType() = default;
VideoNode *ImageType::create(QString arg) {
    auto node = new ImageNode(this);
    if (node) {
        node->setInputCount(inputCount());
        node->setImagePath(name());
    }
    return node;
}

ImageNode::ImageNode(NodeType *nr)
    : VideoNode(nr)
    , m_openGLWorker(new ImageNodeOpenGLWorker(this))
    , m_ready(false) {

    m_periodic.setInterval(10);
    m_periodic.start();
    connect(&m_periodic, &QTimer::timeout, this, &ImageNode::periodic);
    connect(m_openGLWorker.data(), &ImageNodeOpenGLWorker::initialized, this, &ImageNode::onInitialized);
}

ImageNode::ImageNode(const ImageNode &other)
    : VideoNode(other)
    , m_frameTextures(other.m_frameTextures)
    , m_currentTexture(other.m_currentTexture)
    , m_currentTextureIdx(other.m_currentTextureIdx)
    , m_imagePath(other.m_imagePath)
    , m_openGLWorker(other.m_openGLWorker)
    , m_ready(other.m_ready) {
}

ImageNode::~ImageNode() = default;

QString ImageNode::serialize() {
    return m_imagePath;
}

namespace {
std::once_flag reg_once{};
TypeRegistry image_registry{[](NodeRegistry *r) -> QList<NodeType*> {
    std::call_once(reg_once,[](){
        qmlRegisterUncreatableType<ImageNode>("radiance",1,0,"ImageNode","ImageNode must be created through the registry");
    });

    auto res = QList<NodeType*>{};

    QStringList images;
    QDir imgDir(Paths::library() + QString("images/"));
    imgDir.setSorting(QDir::Name);

    for (auto imageName : imgDir.entryList()) {
        if (imageName[0] == '.')
            continue;
        auto t = new ImageType(r);
        if(!t)
            continue;
        t->setName(imageName);
        t->setDescription(imageName);
        t->setInputCount(1);
        res.append(t);
    }
    return res;
}};
}

void ImageNode::onInitialized() {
    m_ready = true;
}

void ImageNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
}

void ImageNode::periodic() {
    Q_ASSERT(QThread::currentThread() == thread());

    // Lock this because we need to use m_frameTextures
    QMutexLocker locker(&m_stateLock);
    if (!m_frameTextures.size())
        return;
    // TODO: actually use m_frameDelays, also this has a discontinuity at MAX_BEAT
    m_currentTextureIdx = (int) (6.0 * timebase->beat()) % m_frameTextures.size();
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
QSharedPointer<VideoNode> ImageNode::createCopyForRendering(QSharedPointer<Chain> chain) {
    Q_UNUSED(chain);
    //periodic();
    return QSharedPointer<VideoNode>(new ImageNode(*this));
}

GLuint ImageNode::paint(QSharedPointer<Chain> chain, QVector<GLuint> inputTextures) {
    if (!m_ready || !m_currentTexture) return 0;
    return m_currentTexture->textureId();
}

// ImageNodeOpenGLWorker methods

ImageNodeOpenGLWorker::ImageNodeOpenGLWorker(ImageNode *p)
    : OpenGLWorker(p->proto()->registry()->workerContext())
    , m_p(p) {
    connect(this, &ImageNodeOpenGLWorker::message, m_p, &ImageNode::message);
    connect(this, &ImageNodeOpenGLWorker::warning, m_p, &ImageNode::warning);
    connect(this, &ImageNodeOpenGLWorker::fatal,   m_p, &ImageNode::fatal);
    connect(this, &QObject::destroyed, this, &ImageNodeOpenGLWorker::onDestroyed);
}

void ImageNodeOpenGLWorker::initialize() {
    // Lock this because we need to use m_frameTextures
    QMutexLocker locker(&m_p->m_stateLock);

    m_p->m_currentTexture = nullptr;

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
    QString filename = Paths::library() + QString("images/") + imagePath;
    QFile file(filename);

    QFileInfo check_file(filename);
    if (!(check_file.exists() && check_file.isFile())) {
        qWarning() << "Could not find" << filename;
        return false;
    }

    QImageReader imageReader(filename);
    int nFrames = imageReader.imageCount();
    if (nFrames == 0)
        nFrames = 1; // Returns 0 if animation isn't supported

    qDeleteAll(m_p->m_frameTextures.begin(), m_p->m_frameTextures.end());

    m_p->m_frameTextures = QVector<QOpenGLTexture *>(nFrames);
    m_p->m_frameDelays = QVector<int>(nFrames);

    for (int i = 0; i < nFrames; i++) {
        QImage frame = imageReader.read();
        if (frame.isNull()) {
            qWarning() << "Unable to read frame" << i << "of image:" << imageReader.errorString();
            return false;
        }

        m_p->m_frameDelays[i] = imageReader.nextImageDelay();
        m_p->m_frameTextures[i] = new QOpenGLTexture(frame.mirrored(), QOpenGLTexture::MipMapGeneration::GenerateMipMaps);
    }

    glFlush();
    qDebug() << "Successfully loaded image" << filename << "with" << nFrames << "frames";

    return true;
}

void ImageNodeOpenGLWorker::onDestroyed() {
    // For some reason, QOpenGLTexture does not have setParent
    // and so we cannot use Qt object tree deletion semantics
    qDeleteAll(m_p->m_frameTextures.begin(), m_p->m_frameTextures.end());
}
