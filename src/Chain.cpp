#include "Chain.h"
#include "main.h"
#include <memory>

Chain::Chain(QSize size)
    : m_openGLWorker(this)
    , m_initialized(false)
    , m_blankTexture(QOpenGLTexture::Target2D)
    , m_noiseTexture(QOpenGLTexture::Target2D)
    , m_size(size) {
    connect(&m_openGLWorker, &ChainOpenGLWorker::initialized, this, &Chain::onInitialized);

    if (!QMetaObject::invokeMethod(&m_openGLWorker, "initialize"))
        qFatal("Unable to initialize openGLWorker");
}

Chain::~Chain() {
}

void Chain::onInitialized() {
    m_initialized = true;
}

QSize Chain::size() {
    return m_size;
}

GLuint Chain::noiseTexture() {
    if (m_initialized)
        return m_noiseTexture.textureId();
    return 0;
}

GLuint Chain::blankTexture() {
    if (m_initialized)
        return m_blankTexture.textureId();
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

void ChainOpenGLWorker::initialize() {
    makeCurrent();
    createBlankTexture();
    createNoiseTexture();
    emit initialized();
}

void ChainOpenGLWorker::createNoiseTexture() {
    Q_ASSERT(!m_p->m_initialized); // Make sure we are not "live"

    m_p->m_noiseTexture.setSize(m_p->size().width(), m_p->size().height());
    m_p->m_noiseTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_p->m_noiseTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    m_p->m_noiseTexture.setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    m_p->m_noiseTexture.setWrapMode(QOpenGLTexture::Repeat);

    auto byteCount = m_p->size().width() * m_p->size().height() * 4;
    auto data = std::make_unique<uint8_t[]>(byteCount);

    qsrand(1);
    for (int i=0; i<m_p->size().width(); i++) {
        qsrand(i + qrand());
        for(int j=0; j<m_p->size().height(); j++) {
            auto k = i * m_p->size().height() + j;
            data[4*k+0] = qrand();
            data[4*k+1] = qrand();
            data[4*k+2] = qrand();
            data[4*k+3] = qrand();
        }
    }

    //std::generate(&data[0], &data[0] + byteCount, qrand);
    m_p->m_noiseTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);

    glFlush();
}

void ChainOpenGLWorker::createBlankTexture() {
    Q_ASSERT(!m_p->m_initialized); // Make sure we are not "live"

    m_p->m_blankTexture.setSize(1, 1);
    m_p->m_blankTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_p->m_blankTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    m_p->m_blankTexture.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_p->m_blankTexture.setWrapMode(QOpenGLTexture::Repeat);

    auto data = std::make_unique<uint8_t[]>(4);
    m_p->m_blankTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
}
