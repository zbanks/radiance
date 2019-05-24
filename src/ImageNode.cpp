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
    : VideoNode(context)
{
    m_openGLWorker = QSharedPointer<ImageNodeOpenGLWorker>(new ImageNodeOpenGLWorker(qSharedPointerCast<ImageNode>(sharedFromThis())), &QObject::deleteLater);

    setInputCount(1);
    setFile(file);
}

QJsonObject ImageNode::serialize() {
    QJsonObject o = VideoNode::serialize();
    o.insert("file", file());
    return o;
}

QString ImageNode::file() {
    QMutexLocker locker(&m_stateLock);
    return m_file;
}

QString ImageNode::fileToName() {
    return QFileInfo(m_file).baseName();
}

QString ImageNode::name() {
    QMutexLocker locker(&m_stateLock);
    return fileToName();
}

void ImageNode::setFile(QString file) {
    file = Paths::contractLibraryPath(file);
    bool wasFileChanged = false;
    bool wasNameChanged = false;
    QString newName;
    {
        QMutexLocker locker(&m_stateLock);
        auto oldName = fileToName();
        if (file != m_file) {
            wasFileChanged = true;
            m_file = file;
            m_ready = false;
            newName = fileToName();
            if (newName != oldName) wasNameChanged = true;
        }
    }
    if (wasFileChanged) {
        setNodeState(VideoNode::Loading);
        bool result = QMetaObject::invokeMethod(m_openGLWorker.data(), "initialize", Q_ARG(QString, file));
        Q_ASSERT(result);
        emit fileChanged(file);
    }
    if (wasNameChanged) emit nameChanged(newName);
}

GLuint ImageNode::paint(ChainSP chain, QVector<GLuint> inputTextures) {
    int totalDelay;
    QVector<int> frameDelays;
    QVector<QSharedPointer<QOpenGLTexture>> frameTextures;
    {
        QMutexLocker locker(&m_stateLock);
        if (!m_ready || m_frameTextures.empty())
            return 0;

        totalDelay = m_totalDelay;
        frameDelays = m_frameDelays;
        frameTextures = m_frameTextures;
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

ImageNodeOpenGLWorker::ImageNodeOpenGLWorker(QSharedPointer<ImageNode> p)
    : OpenGLWorker(p->context()->openGLWorkerContext())
    , m_p(p) {
    connect(this, &ImageNodeOpenGLWorker::message, p.data(), &ImageNode::message);
    connect(this, &ImageNodeOpenGLWorker::warning, p.data(), &ImageNode::warning);
    connect(this, &ImageNodeOpenGLWorker::error,   p.data(), &ImageNode::error);
}

void ImageNodeOpenGLWorker::initialize(QString filename) {
    auto p = m_p.toStrongRef();
    if (p.isNull()) return; // ImageNode was deleted

    filename = Paths::expandLibraryPath(filename);
    QFile file(filename);

    QFileInfo check_file(filename);
    if (!(check_file.exists() && check_file.isFile())) {
        emit error(QString("Could not find \"%1\"").arg(filename));
        p->setNodeState(VideoNode::Broken);
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
            emit error(QString("Unable to read frame %1 of image: %2").arg(i).arg(imageReader.errorString()));
            p->setNodeState(VideoNode::Broken);
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
        QMutexLocker locker(&p->m_stateLock);
        p->m_totalDelay = totalDelay;
        p->m_frameTextures = frameTextures;
        p->m_frameDelays = frameDelays;
        p->m_ready = true;
    }
    glFlush();
    p->setNodeState(VideoNode::Ready);
}

QString ImageNode::typeName() {
    return "ImageNode";
}

VideoNodeSP *ImageNode::deserialize(Context *context, QJsonObject obj) {
    QString name = obj.value("file").toString();
    if (obj.isEmpty()) {
        return nullptr;
    }
    return new VideoNodeSP(new ImageNode(context, name));
}

bool ImageNode::canCreateFromFile(QString filename) {
    return QImageReader(filename).canRead();
}

VideoNodeSP *ImageNode::fromFile(Context *context, QString filename) {
    return new VideoNodeSP(new ImageNode(context, filename));
}

QMap<QString, QString> ImageNode::customInstantiators() {
    return QMap<QString, QString>();
}
