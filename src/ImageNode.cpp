#include "ImageNode.h"
#include "RenderContext.h"
#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>
#include <memory>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLFramebufferObject>
#include "main.h"

static bool ilInitted = false;

ImageNode::ImageNode()
    : VideoNode(renderContext)
    , m_openGLWorker(new ImageNodeOpenGLWorker(this))
    , m_ticksToNextFrame(0) {

    connect(m_context.data(), &RenderContext::periodic, this, &ImageNode::periodic);
    connect(m_openGLWorker.data(), &ImageNodeOpenGLWorker::initialized, this, &ImageNode::onInitialized);
}

ImageNode::ImageNode(const ImageNode &other)
    : VideoNode(other)
    , m_openGLWorker(other.m_openGLWorker)
    , m_currentTexture(other.m_currentTexture)
    , m_currentTextureIdx(other.m_currentTextureIdx)
    , m_frameTextures(other.m_frameTextures) {
}

ImageNode::~ImageNode() {
}

void ImageNode::initialize() {
}

void ImageNode::onInitialized() {
    setReady(true);
}

void ImageNode::periodic() {
    Q_ASSERT(QThread::currentThread() == thread());

    // Lock this because we need to adjust the frames
    QMutexLocker locker(&m_stateLock);

    m_ticksToNextFrame = (++m_ticksToNextFrame) % TICKS_PER_FRAME;

    if (m_ticksToNextFrame == 0) {
        m_currentTextureIdx = (++m_currentTextureIdx) % m_frameTextures.size();
        m_currentTexture = m_frameTextures.at(m_currentTextureIdx);
    }
}

QString ImageNode::imagePath() {
    Q_ASSERT(QThread::currentThread() == thread());
    return m_imagePath;
}

void ImageNode::setImagePath(QString imagePath) {
    Q_ASSERT(QThread::currentThread() == thread());
    if(imagePath != m_imagePath) {
        setReady(false);
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

GLuint ImageNode::paint(int chain, QVector<GLuint> inputTextures) {
    return m_currentTexture;
}

void ImageNode::copyBackRenderState(int chain, QSharedPointer<VideoNode> copy) {
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
    makeCurrent();
    bool result = loadImage(m_p->m_imagePath);
    if (!result) {
        return;
    }

    // Set the current frame to 0
    m_p->m_currentTexture = m_p->m_frameTextures.at(0);
    m_p->m_currentTextureIdx = 0;

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

    if (!ilInitted) {
        // See http://openil.sourceforge.net/docs/DevIL%20Manual.pdf for this sequence
        ilInit();
        iluInit();
        ilutRenderer(ILUT_OPENGL);

        ilInitted = true;
    }

    // Give ourselves an image name to bind to.
    ILuint imageInfo;
    ilGenImages(1, &imageInfo);
    ilBindImage(imageInfo);

    if (!ilLoadImage(qPrintable(filename))) {
        qCritical() << "Could not load image" << filename << ": " << iluErrorString(ilGetError());
        return false;
    }

    int nFrames = ilGetInteger(IL_NUM_IMAGES) + 1;
    qDebug() << "Found" << nFrames << "frames in image" << filename;

    m_p->m_frameTextures.resize(nFrames);
    m_p->m_frameTextures.squeeze();

    for (int i = 0; i < nFrames; i++) {
        ILenum bindError;

        // It's really important to call this each time or it has trouble loading frames
        ilBindImage(imageInfo);

        if (ilActiveImage(i) == IL_FALSE) {
            qCritical() << "Error setting active frame" << i << "of image: " << iluErrorString(ilGetError());

            return false;
        }

        m_p->m_frameTextures[i] = ilutGLBindTexImage();

        bindError = ilGetError();

        if (bindError != IL_NO_ERROR) {
            qCritical() << "Error binding frame" << i << "of image: " << iluErrorString(bindError);
            return false;
        }
    }

    qDebug() << "Successfully loaded image" << filename;

    // Now that it's been loaded into OpenGL delete it.
    ilDeleteImages(1, &imageInfo);
}
