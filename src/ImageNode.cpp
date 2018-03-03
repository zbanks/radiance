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

ImageNode::ImageNode(Context *context, QString file)
    : VideoNode(new ImageNodePrivate(context))
{
    setInputCount(1);
    setFile(file);
}

ImageNode::ImageNode(const ImageNode &other)
    : VideoNode(other)
{
}

ImageNode *ImageNode::clone() const {
    return new ImageNode(*this);
}

QSharedPointer<ImageNodePrivate> ImageNode::d() {
    return d_ptr.staticCast<ImageNodePrivate>();
}

QJsonObject ImageNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("file", file());
    return o;
}

void ImageNode::onInitialized() {
    QMutexLocker locker(&d()->m_stateLock);
    d()->m_ready = true;
}

void ImageNode::chainsEdited(QList<Chain> added, QList<Chain> removed) {
}
QString ImageNode::file() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_file;
}

QString ImageNode::fileToName() {
    return QFileInfo(d()->m_file).baseName();
}

QString ImageNode::name() {
    QMutexLocker locker(&d()->m_stateLock);
    return fileToName();
}

void ImageNode::setFile(QString file) {
    file = Paths::contractLibraryPath(file);
    bool wasFileChanged = false;
    bool wasNameChanged = false;
    QString newName;
    {
        QMutexLocker locker(&d()->m_stateLock);
        auto oldName = fileToName();
        if (file != d()->m_file) {
            wasFileChanged = true;
            d()->m_file = file;
            d()->m_openGLWorker = QSharedPointer<ImageNodeOpenGLWorker>(new ImageNodeOpenGLWorker(this, d()->m_file), &QObject::deleteLater);
            connect(d()->m_openGLWorker.data(), &ImageNodeOpenGLWorker::initialized, this, &ImageNode::onInitialized);
            bool result = QMetaObject::invokeMethod(d()->m_openGLWorker.data(), "initialize");
            Q_ASSERT(result);
            newName = fileToName();
            if (newName != oldName) wasNameChanged = true;
        }
    }
    if (wasFileChanged) emit fileChanged(file);
    if (wasNameChanged) emit nameChanged(newName);
}

GLuint ImageNode::paint(Chain chain, QVector<GLuint> inputTextures) {
    if (!d()->m_openGLWorker || !d()->m_openGLWorker->m_ready.load() || !d()->m_openGLWorker->m_frameTextures.size())
        return 0;

    // This seems not the most thread-safe...

    auto currentMs = int64_t(context()->timebase()->beat() *  500);
    auto extraMs   = currentMs;
    if(d()->m_openGLWorker->m_totalDelay) {
        extraMs   = currentMs % (d()->m_openGLWorker->m_totalDelay);
        if(!d()->m_openGLWorker->m_frameDelays.empty()) {
            for(auto i = 0; i < d()->m_openGLWorker->m_frameDelays.size(); ++i) {
                if(d()->m_openGLWorker->m_frameDelays.at(i) >= extraMs)
                    return d()->m_openGLWorker->m_frameTextures.at(i)->textureId();
            }
        } else {
            auto perFrame = d()->m_openGLWorker->m_totalDelay / d()->m_openGLWorker->m_frameTextures.size();;
            auto idx = std::min((unsigned long)(extraMs / perFrame), (unsigned long)(d()->m_openGLWorker->m_frameTextures.size() - 1));
            return d()->m_openGLWorker->m_frameTextures.at(idx)->textureId();
        }
    }
    return 0;
}

// ImageNodeOpenGLWorker methods

ImageNodeOpenGLWorker::ImageNodeOpenGLWorker(ImageNode*p, QString file)
    : OpenGLWorker(p->context()->openGLWorkerContext())
    , m_file(file) {
    connect(this, &ImageNodeOpenGLWorker::message, p, &ImageNode::message);
    connect(this, &ImageNodeOpenGLWorker::warning, p, &ImageNode::warning);
    connect(this, &ImageNodeOpenGLWorker::fatal,   p, &ImageNode::fatal);
}

ImageNodeOpenGLWorker::~ImageNodeOpenGLWorker() {
    makeCurrent();
}

bool ImageNodeOpenGLWorker::ready() const {
    return m_ready.load();
}

void ImageNodeOpenGLWorker::initialize() {
    // Lock this because we need to use m_frameTextures
    makeCurrent();
    auto result = loadImage(m_file);
    if (!result) {
        qWarning() << "Can't load image!" << m_file;
        return;
    }
    m_ready.store(true);
    // Set the current frame to 0
    emit initialized();
}

// Call this to load an image into m_frameTextures
// Returns true if the program was loaded successfully
bool ImageNodeOpenGLWorker::loadImage(QString filename) {
    filename = Paths::expandLibraryPath(filename);
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

QString ImageNode::typeName() {
    return "ImageNode";
}

VideoNode *ImageNode::deserialize(Context *context, QJsonObject obj) {
    QString name = obj.value("file").toString();
    if (obj.isEmpty()) {
        return nullptr;
    }
    ImageNode *e = new ImageNode(context, name);
    return e;
}

bool ImageNode::canCreateFromFile(QString filename) {
    return QImageReader(filename).canRead();
}

VideoNode *ImageNode::fromFile(Context *context, QString filename) {
    ImageNode *e = new ImageNode(context, filename);
    return e;
}

QMap<QString, QString> ImageNode::customInstantiators() {
    return QMap<QString, QString>();
}

ImageNodePrivate::ImageNodePrivate(Context *context)
    : VideoNodePrivate(context)
{
}
