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
    d()->m_openGLWorker = QSharedPointer<ImageNodeOpenGLWorker>(new ImageNodeOpenGLWorker(*this), &QObject::deleteLater);

    setInputCount(1);
    setFile(file);
}

ImageNode::ImageNode(const ImageNode &other)
    : VideoNode(other)
{
}

ImageNode::ImageNode(QSharedPointer<ImageNodePrivate> other_ptr)
    : VideoNode(other_ptr.staticCast<VideoNodePrivate>())
{
}

ImageNode *ImageNode::clone() const {
    return new ImageNode(*this);
}

QSharedPointer<ImageNodePrivate> ImageNode::d() const {
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
            d()->m_ready = false;
            bool result = QMetaObject::invokeMethod(d()->m_openGLWorker.data(), "initialize", Q_ARG(QString, file));
            Q_ASSERT(result);
            newName = fileToName();
            if (newName != oldName) wasNameChanged = true;
        }
    }
    if (wasFileChanged) emit fileChanged(file);
    if (wasNameChanged) emit nameChanged(newName);
}

GLuint ImageNode::paint(Chain chain, QVector<GLuint> inputTextures) {
    int totalDelay;
    QVector<int> frameDelays;
    QVector<QSharedPointer<QOpenGLTexture>> frameTextures;
    {
        QMutexLocker locker(&d()->m_stateLock);
        if (!d()->m_ready || d()->m_frameTextures.empty())
            return 0;

        totalDelay = d()->m_totalDelay;
        frameDelays = d()->m_frameDelays;
        frameTextures = d()->m_frameTextures;
    }

    auto currentMs = int64_t(context()->timebase()->beat() *  500);
    auto extraMs = currentMs;
    if (totalDelay) {
        extraMs = currentMs % totalDelay;
        if (frameDelays.empty()) {
            for (auto i = 0; i < frameDelays.size(); ++i) {
                if (frameDelays.at(i) >= extraMs)
                    return frameTextures.at(i)->textureId();
            }
        } else {
            auto perFrame = totalDelay / frameTextures.size();;
            auto idx = std::min((unsigned long)(extraMs / perFrame), (unsigned long)(frameTextures.size() - 1));
            return frameTextures.at(idx)->textureId();
        }
    }
    return 0;
}

// ImageNodeOpenGLWorker methods

ImageNodeOpenGLWorker::ImageNodeOpenGLWorker(ImageNode p)
    : OpenGLWorker(p.context()->openGLWorkerContext())
    , m_p(p) {
    connect(this, &ImageNodeOpenGLWorker::message, &p, &ImageNode::message);
    connect(this, &ImageNodeOpenGLWorker::warning, &p, &ImageNode::warning);
    connect(this, &ImageNodeOpenGLWorker::fatal,   &p, &ImageNode::fatal);
}

void ImageNodeOpenGLWorker::initialize(QString filename) {
    auto d = m_p.toStrongRef();
    if (d.isNull()) return; // ImageNode was deleted
    ImageNode p(d);

    filename = Paths::expandLibraryPath(filename);
    QFile file(filename);

    QFileInfo check_file(filename);
    if (!(check_file.exists() && check_file.isFile())) {
        emit fatal(QString("Could not find \"%1\"").arg(filename));
    }

    QImageReader imageReader(filename);
    int nFrames = imageReader.imageCount();
    if (nFrames == 0)
        nFrames = 1; // Returns 0 if animation isn't supported

    int totalDelay;
    QVector<int> frameDelays;
    QVector<QSharedPointer<QOpenGLTexture>> frameTextures;

    frameTextures.resize(nFrames);
    frameDelays.resize(nFrames);

    for (int i = 0; i < nFrames; i++) {
        auto frame = imageReader.read();
        if (frame.isNull()) {
            emit fatal(QString("Unable to read frame %1 of image: %2").arg(i).arg(imageReader.errorString()));
            return;
        }

        frameDelays[i] = imageReader.nextImageDelay();
        if(!frameDelays[i])
            frameDelays[i] = 10;
        frameTextures[i] = QSharedPointer<QOpenGLTexture>::create(frame.mirrored(), QOpenGLTexture::MipMapGeneration::GenerateMipMaps);
    }
    std::partial_sum(frameDelays.begin(),frameDelays.end(),frameDelays.begin());
    totalDelay = frameDelays.back();
    qDebug() << "Successfully loaded image " << filename << " with " << nFrames << "frames, and a total delay of " << totalDelay << "ms";

    {
        QMutexLocker locker(&p.d()->m_stateLock);
        p.d()->m_totalDelay = totalDelay;
        p.d()->m_frameTextures = frameTextures;
        p.d()->m_frameDelays = frameDelays;
        p.d()->m_ready = true;
    }
    glFlush();
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

WeakImageNode::WeakImageNode()
{
}

WeakImageNode::WeakImageNode(const ImageNode &other)
    : d_ptr(other.d())
{
}

QSharedPointer<ImageNodePrivate> WeakImageNode::toStrongRef() {
    return d_ptr.toStrongRef();
}


ImageNodePrivate::ImageNodePrivate(Context *context)
    : VideoNodePrivate(context)
{
}
