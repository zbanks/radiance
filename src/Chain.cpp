#include "Chain.h"
#include <memory>
#include <array>
#include <algorithm>
#include "xoroshiro128plus.hpp"
#include <QDebug>

using namespace Xoroshiro;

Chain::Chain(QSize size)
    : d_ptr(new ChainPrivate(size), &QObject::deleteLater)
{
}

Chain::Chain(const Chain &other)
    : d_ptr(other.d_ptr)
{
}

Chain::operator QString() const {
    return QString("Chain(%1x%2, %3)").arg(d_ptr->m_size.width()).arg(d_ptr->m_size.height()).arg(thread()->objectName());
}

QSize Chain::size() {
    return d_ptr->m_size;
}

GLuint Chain::noiseTexture() {
    if (!d_ptr->m_noiseTexture.isCreated()) {
        d_ptr->m_noiseTexture.setSize(d_ptr->m_size.width(), d_ptr->m_size.height());
        d_ptr->m_noiseTexture.setFormat(QOpenGLTexture::RGBA32F);
        d_ptr->m_noiseTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);
        d_ptr->m_noiseTexture.setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        d_ptr->m_noiseTexture.setWrapMode(QOpenGLTexture::Repeat);

        auto compCount = d_ptr->m_size.width() * d_ptr->m_size.height() * 4;
        auto data = std::make_unique<float[]>(compCount);

        auto xsr = xoroshiro128plus_engine(reinterpret_cast<uint64_t>(this));

        auto xsrd = [&xsr, div = (1./(UINT64_C(1)<<53))](){
            return (xsr() >> 11) * div;
        };
        std::generate(&data[0],&data[compCount],xsrd);
        d_ptr->m_noiseTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, &data[0]);
        glFlush();

        if (QThread::currentThread() != d_ptr->thread()) {
            d_ptr->moveToThread(QThread::currentThread());
        }
    }

    return d_ptr->m_noiseTexture.textureId();
}

GLuint Chain::blankTexture() {
    if (!d_ptr->m_blankTexture.isCreated()) {
        d_ptr->m_blankTexture.setSize(1, 1);
        d_ptr->m_blankTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
        d_ptr->m_blankTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
        d_ptr->m_blankTexture.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
        d_ptr->m_blankTexture.setWrapMode(QOpenGLTexture::Repeat);

        auto data = std::array<uint8_t,4>();
        d_ptr->m_blankTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);

        if (QThread::currentThread() != d_ptr->thread()) {
            d_ptr->moveToThread(QThread::currentThread());
        }
    }

    return d_ptr->m_blankTexture.textureId();
}

QOpenGLVertexArrayObject *Chain::vao() {
    if (!d_ptr->m_vao.isCreated()) {
        d_ptr->m_vao.create();

        if (QThread::currentThread() != d_ptr->thread()) {
            d_ptr->moveToThread(QThread::currentThread());
        }
    }
    return &d_ptr->m_vao;
}

bool Chain::operator==(const Chain &other) const {
    return d_ptr == other.d_ptr;
}

bool Chain::operator>(const Chain &other) const {
    return d_ptr.data() > other.d_ptr.data();
}

bool Chain::operator<(const Chain &other) const {
    return d_ptr.data() > other.d_ptr.data();
}

Chain &Chain::operator=(const Chain &other) {
    d_ptr = other.d_ptr;
    return *this;
}

ChainPrivate::ChainPrivate(QSize size)
    : m_noiseTexture(QOpenGLTexture::Target2D)
    , m_blankTexture(QOpenGLTexture::Target2D)
    , m_vao(new QOpenGLVertexArrayObject())
    , m_size(size)
{
}

ChainPrivate::~ChainPrivate() {
}
