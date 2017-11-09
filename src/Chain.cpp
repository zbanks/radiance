#include "Chain.h"
#include "main.h"
#include <memory>
#include <array>
#include <algorithm>
#include "xoroshiro128plus.hpp"

using namespace Xoroshiro;

Chain::Chain(QSize size)
    :m_openGLWorker(new ChainOpenGLWorker(this))
    , m_size(size) {
    connect(m_openGLWorker, &ChainOpenGLWorker::initialized, this, &Chain::onInitialized);
    connect(this, &QObject::destroyed, m_openGLWorker, &QObject::deleteLater);

    if (!QMetaObject::invokeMethod(m_openGLWorker, "initialize", Q_ARG(QSize, m_size)))
        qFatal("Unable to initialize openGLWorker");
}

Chain::~Chain() = default;

void Chain::onInitialized(int n, int b) {
    m_noiseTextureId = n;
    m_blankTextureId = b;
    m_initialized = true;
}

QSize Chain::size() {
    return m_size;
}

qreal Chain::beatTime() const {
    return m_beatTime;
};
qreal Chain::realTime() const {
    return m_realTime;
};
void Chain::setBeatTime(qreal _time) {
    m_beatTime = _time;
}
void Chain::setRealTime(qreal _time) {
    m_realTime = _time;
}
GLuint Chain::noiseTexture() const {
    return m_noiseTextureId;//.textureId();
}

GLuint Chain::blankTexture() const {
    return m_blankTextureId;//.textureId();
}

QOpenGLVertexArrayObject &Chain::vao() {
    return m_vao;
}
const QOpenGLVertexArrayObject &Chain::vao() const {
    return m_vao;
}
// ChainOpenGLWorker methods

ChainOpenGLWorker::ChainOpenGLWorker(Chain *p)
    : OpenGLWorker(openGLWorkerContext) {
}

void ChainOpenGLWorker::initialize(QSize size) {
    makeCurrent();
    createBlankTexture(size);
    createNoiseTexture(size);
    emit initialized(m_noiseTexture.textureId(),m_blankTexture.textureId());
}

void ChainOpenGLWorker::createNoiseTexture(QSize size) {
    m_noiseTexture.setSize(size.width(), size.height());
    m_noiseTexture.setFormat(QOpenGLTexture::RGBA32F);
    m_noiseTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
    m_noiseTexture.setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    m_noiseTexture.setWrapMode(QOpenGLTexture::Repeat);

    auto compCount = size.width() * size.height() * 4;
    auto data = std::make_unique<float[]>(compCount);

    auto xsr = xoroshiro128plus_engine(reinterpret_cast<uint64_t>(this));

    auto xsrd = [&xsr, div = (1./(UINT64_C(1)<<53))](){
        return (xsr() >> 11) * div;
    };
    std::generate(&data[0],&data[compCount],xsrd);
    m_noiseTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, &data[0]);
    glFlush();
}

void ChainOpenGLWorker::createBlankTexture(QSize size) {
    m_blankTexture.setSize(1, 1);
    m_blankTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_blankTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    m_blankTexture.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_blankTexture.setWrapMode(QOpenGLTexture::Repeat);

    auto data = std::array<uint8_t,4>();
    m_blankTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
}
