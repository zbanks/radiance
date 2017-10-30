#include "Chain.h"
#include "main.h"
#include <memory>

Chain::Chain(QSize size)
    : m_initialized(false)
    , m_openGLWorker(new ChainOpenGLWorker(this))
    , m_size(size) {
    connect(m_openGLWorker, &ChainOpenGLWorker::initialized, this, &Chain::onInitialized);
    connect(this, &QObject::destroyed, m_openGLWorker, &QObject::deleteLater);

    if (!QMetaObject::invokeMethod(m_openGLWorker, "initialize", Q_ARG(QSize, m_size)))
        qFatal("Unable to initialize openGLWorker");
}

Chain::~Chain() {
}

void Chain::onInitialized(int n, int b) {
    m_noiseTextureId = n;
    m_blankTextureId = b;
    m_initialized = true;
}

QSize Chain::size() {
    return m_size;
}

GLuint Chain::noiseTexture() {
    if (m_initialized)
        return m_noiseTextureId;//.textureId();
    return 0;
}

GLuint Chain::blankTexture() {
    if (m_initialized)
        return m_blankTextureId;//.textureId();
    return 0;
}

QOpenGLVertexArrayObject &Chain::vao() {
    return m_vao;
}
const QOpenGLVertexArrayObject &Chain::vao() const {
    return m_vao;
}
// ChainOpenGLWorker methods

ChainOpenGLWorker::ChainOpenGLWorker(Chain *p)
    : OpenGLWorker(openGLWorkerContext)
    , m_p(p) {
}

void ChainOpenGLWorker::initialize(QSize size) {
    makeCurrent();
    createBlankTexture(size);
    createNoiseTexture(size);
    emit initialized(m_noiseTexture.textureId(),m_blankTexture.textureId());
}

void ChainOpenGLWorker::createNoiseTexture(QSize size) {
    Q_ASSERT(!m_p->m_initialized); // Make sure we are not "live"

    m_noiseTexture.setSize(size.width(), size.height());
    m_noiseTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_noiseTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    m_noiseTexture.setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    m_noiseTexture.setWrapMode(QOpenGLTexture::Repeat);

    auto byteCount = size.width() * size.height() * 4;
    auto data = std::make_unique<uint8_t[]>(byteCount);

    qsrand(1);
    for (int i=0; i<size.width(); i++) {
        qsrand(i + qrand());
        for(int j=0; j<size.height(); j++) {
            auto k = i * size.height() + j;
            data[4*k+0] = qrand();
            data[4*k+1] = qrand();
            data[4*k+2] = qrand();
            data[4*k+3] = qrand();
        }
    }

    //std::generate(&data[0], &data[0] + byteCount, qrand);
    m_noiseTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);

    glFlush();
}

void ChainOpenGLWorker::createBlankTexture(QSize size) {
    Q_ASSERT(!m_p->m_initialized); // Make sure we are not "live"

    m_blankTexture.setSize(1, 1);
    m_blankTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_blankTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    m_blankTexture.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_blankTexture.setWrapMode(QOpenGLTexture::Repeat);

    auto data = std::make_unique<uint8_t[]>(4);
    m_blankTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
}
