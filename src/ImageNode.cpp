#include "ImageNode.h"
#include "Context.h"
#include "Audio.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QtQml>
#include "Paths.h"

ImageNode::ImageNode(Context *context, QString imagePath)
    : VideoNode(context)
    , m_ready(false) {
    setInputCount(1);
    setImagePath(imagePath);
}

ImageNode::ImageNode(const ImageNode &other)
    : VideoNode(other)
//    , m_frameTextures(other.m_frameTextures)
//    , m_currentTexture(other.m_currentTexture)
//    , m_currentTextureIdx(other.m_currentTextureIdx)
    , m_imagePath(other.m_imagePath)
    , m_openGLWorker(other.m_openGLWorker)
    , m_ready(other.m_ready) {
}

ImageNode::~ImageNode() = default;

QJsonObject ImageNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("imagePath", m_imagePath);
    return o;
}

void ImageNode::onInitialized() {
    m_ready = true;
}

void ImageNode::chainsEdited(QList<QSharedPointer<Chain>> added, QList<QSharedPointer<Chain>> removed) {
}
QString ImageNode::imagePath() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_imagePath;
}

void ImageNode::setImagePath(QString imagePath) {
    Q_ASSERT(QThread::currentThread() == thread());
    if(imagePath != m_imagePath) {
        {
            QMutexLocker locker(&m_stateLock);
            m_imagePath = imagePath;
            m_openGLWorker = QSharedPointer<ImageNodeOpenGLWorker>(new ImageNodeOpenGLWorker(this, m_imagePath), &QObject::deleteLater);
            connect(m_openGLWorker.data(), &ImageNodeOpenGLWorker::initialized, this, &ImageNode::onInitialized);
            bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "initialize");
            Q_ASSERT(result);

        }
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
    if (!m_openGLWorker || !m_openGLWorker->m_ready.load() || !m_openGLWorker->m_frameTextures.size())
        return 0;

    auto currentMs = int64_t(context()->audio()->time() *  5e3); // XXX IDK wtf time() is, but this speed factor makes the nyancat look good
    auto extraMs   = currentMs;
    if(m_openGLWorker->m_totalDelay) {
        extraMs   = currentMs % (m_openGLWorker->m_totalDelay);
        if(!m_openGLWorker->m_frameDelays.empty()) {
            for(auto i = size_t{}; i < m_openGLWorker->m_frameDelays.size(); ++i) {
                if(m_openGLWorker->m_frameDelays.at(i) >= extraMs)
                    return m_openGLWorker->m_frameTextures.at(i)->textureId();
            }
        } else {
            auto perFrame = m_openGLWorker->m_totalDelay / m_openGLWorker->m_frameTextures.size();;
            auto idx = std::min((unsigned long)(extraMs / perFrame), (unsigned long)(m_openGLWorker->m_frameTextures.size() - 1));
            return m_openGLWorker->m_frameTextures.at(idx)->textureId();
        }
    }
    return 0;
}

// ImageNodeOpenGLWorker methods

ImageNodeOpenGLWorker::ImageNodeOpenGLWorker(ImageNode*p, QString imagePath)
    : OpenGLWorker(p->context()->openGLWorkerContext())
    , m_imagePath(imagePath) {
//    connect(this, &ImageNodeOpenGLWorker::message, p, &ImageNode::message);
//    connect(this, &ImageNodeOpenGLWorker::warning, p, &ImageNode::warning);
//    connect(this, &ImageNodeOpenGLWorker::fatal,   p, &ImageNode::fatal);
    connect(this, &QObject::destroyed, this, &ImageNodeOpenGLWorker::onDestroyed);
}
ImageNodeOpenGLWorker::~ImageNodeOpenGLWorker() {
    makeCurrent();
//    m_frameTextures.clear();
//    m_frameDelays.clear();
}
bool ImageNodeOpenGLWorker::ready() const {
    return m_ready.load();
}
void ImageNodeOpenGLWorker::initialize() {
    // Lock this because we need to use m_frameTextures
    makeCurrent();
    auto result = loadImage(m_imagePath);
    if (!result) {
        qWarning() << "Can't load image!" << m_imagePath;
        return;
    }
    m_ready.store(true);
    // Set the current frame to 0
    emit initialized();
}

// Call this to load an image into m_frameTextures
// Returns true if the program was loaded successfully
bool ImageNodeOpenGLWorker::loadImage(QString imagePath) {
    auto filename = Paths::library() + QString("images/") + imagePath;
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

    m_frameTextures.clear();
    m_frameTextures.resize(nFrames);
    m_frameDelays.resize(nFrames);

    for (int i = 0; i < nFrames; i++) {
        auto frame = imageReader.read();
        if (frame.isNull()) {
            qWarning() << "Unable to read frame" << i << "of image:" << imageReader.errorString();
            return false;
        }

        m_frameDelays[i] = imageReader.nextImageDelay();
        if(!m_frameDelays[i])
            m_frameDelays[i] = 10;
        m_frameTextures[i] = QSharedPointer<QOpenGLTexture>::create(frame.mirrored(), QOpenGLTexture::MipMapGeneration::GenerateMipMaps);
    }
    std::partial_sum(m_frameDelays.begin(),m_frameDelays.end(),m_frameDelays.begin());
    m_totalDelay = m_frameDelays.back();
    glFlush();
    qDebug() << "Successfully loaded image " << filename << " with " << nFrames << "frames, and a total delay of " << m_totalDelay << "ms";

    return true;
}

void ImageNodeOpenGLWorker::onDestroyed() {
    // For some reason, QOpenGLTexture does not have setParent
    // and so we cannot use Qt object tree deletion semantics
}

QString ImageNode::typeName() {
    return "ImageNode";
}

VideoNode *ImageNode::deserialize(Context *context, QJsonObject obj) {
    QString name = obj.value("imagePath").toString();
    if (obj.isEmpty()) {
        return nullptr;
    }
    ImageNode *e = new ImageNode(context, name);
    return e;
}

bool ImageNode::canCreateFromFile(QString filename) {
    return filename.endsWith(".gif", Qt::CaseInsensitive);
}

VideoNode *ImageNode::fromFile(Context *context, QString filename) {
    ImageNode *e = new ImageNode(context, filename);
    return e;
}

