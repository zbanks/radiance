#include "Chain.h"
#include <memory>
#include <array>
#include <algorithm>
#include "xoroshiro128plus.hpp"

using namespace Xoroshiro;

Chain::Chain(QSize size)
    : m_noiseTexture(QOpenGLTexture::Target2D)
    , m_blankTexture(QOpenGLTexture::Target2D)
    , m_size(size)
    {
}

Chain::~Chain() = default;

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
GLuint Chain::noiseTexture() {
    if (!m_noiseTexture.isCreated()) {
        m_noiseTexture.setSize(m_size.width(), m_size.height());
        m_noiseTexture.setFormat(QOpenGLTexture::RGBA32F);
        m_noiseTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
        m_noiseTexture.setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        m_noiseTexture.setWrapMode(QOpenGLTexture::Repeat);

        auto compCount = m_size.width() * m_size.height() * 4;
        auto data = std::make_unique<float[]>(compCount);

        auto xsr = xoroshiro128plus_engine(reinterpret_cast<uint64_t>(this));

        auto xsrd = [&xsr, div = (1./(UINT64_C(1)<<53))](){
            return (xsr() >> 11) * div;
        };
        std::generate(&data[0],&data[compCount],xsrd);
        m_noiseTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, &data[0]);
        glFlush();
    }

    return m_noiseTexture.textureId();
}

GLuint Chain::blankTexture() {
    if (!m_blankTexture.isCreated()) {
        m_blankTexture.setSize(1, 1);
        m_blankTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
        m_blankTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
        m_blankTexture.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
        m_blankTexture.setWrapMode(QOpenGLTexture::Repeat);

        auto data = std::array<uint8_t,4>();
        m_blankTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
    }

    return m_blankTexture.textureId();
}

QOpenGLVertexArrayObject &Chain::vao() {
    return m_vao;
}
const QOpenGLVertexArrayObject &Chain::vao() const {
    return m_vao;
}
