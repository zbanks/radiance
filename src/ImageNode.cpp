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
    , m_ready(false) {
    if(auto it = qobject_cast<ImageType*>(nr)) {
        m_openGLWorker = it->m_openGLWorker;
        if(m_openGLWorker) {
            if(m_openGLWorker->ready())
                m_ready = true;
            connect(m_openGLWorker.data(), &ImageNodeOpenGLWorker::initialized, this, &ImageNode::onInitialized);
        }
    }
//    m_periodic.setInterval(10);
//    m_periodic.start();
//    connect(&m_periodic, &QTimer::timeout, this, &ImageNode::periodic);
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
        t->m_openGLWorker = QSharedPointer<ImageNodeOpenGLWorker>(new ImageNodeOpenGLWorker(t, t->name()),&QObject::deleteLater);
        bool result = QMetaObject::invokeMethod(t->m_openGLWorker.data(), "initialize");
        Q_ASSERT(result);

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

    auto currentMs = int64_t(chain->realTime() *  1e3);
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

ImageNodeOpenGLWorker::ImageNodeOpenGLWorker(ImageType *p, QString imagePath)
    : OpenGLWorker(p->registry()->workerContext())
    , m_imagePath(imagePath) {
//    connect(this, &ImageNodeOpenGLWorker::message, p, &ImageNode::message);
//    connect(this, &ImageNodeOpenGLWorker::warning, p, &ImageNode::warning);
//    connect(this, &ImageNodeOpenGLWorker::fatal,   p, &ImageNode::fatal);
    connect(this, &QObject::destroyed, this, &ImageNodeOpenGLWorker::onDestroyed);
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
    makeCurrent();
    m_frameTextures.clear();
    m_frameDelays.clear();
}
