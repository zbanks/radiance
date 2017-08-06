#include "RenderContext.h"
#include <memory>
#include "Model.h"

void RenderContext::createNoiseTextures() {
    m_noiseTextures.clear();
    for(int i=0; i<chainCount(); i++) {
        auto tex = QSharedPointer<QOpenGLTexture>(new QOpenGLTexture(QOpenGLTexture::Target2D));
        tex->setSize(chainSize(i).width(), chainSize(i).height());
        tex->setFormat(QOpenGLTexture::RGBA8_UNorm);
        tex->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
        tex->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        tex->setWrapMode(QOpenGLTexture::Repeat);

        auto byteCount = chainSize(i).width() * chainSize(i).height() * 4;
        auto data = std::make_unique<uint8_t[]>(byteCount);
        qsrand(1);
        std::generate(&data[0],&data[0] + byteCount,qrand);
        tex->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
        m_noiseTextures.append(tex);
    }
}


void RenderContext::createBlankTexture() {
    m_blankTexture = QSharedPointer<QOpenGLTexture>(new QOpenGLTexture(QOpenGLTexture::Target2D));
    m_blankTexture->setSize(1, 1);
    m_blankTexture->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_blankTexture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    m_blankTexture->setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_blankTexture->setWrapMode(QOpenGLTexture::Repeat);

    auto data = std::make_unique<uint8_t[]>(4);
    m_blankTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
}

RenderContext::RenderContext() {
}

RenderContext::~RenderContext() {
}

void RenderContext::initialize() {
    m_initialized = false;
    createBlankTexture();
    createNoiseTextures();
    m_initialized = true;
}

QOpenGLTexture *RenderContext::texture(int chain, VideoNode *videoNode) {
    return nullptr; // TODO
}

int RenderContext::chainCount() {
    return 2;
}

QSize RenderContext::chainSize(int chain) {
    switch(chain) {
        case 0:
            return QSize(300, 300);
        case 1:
            return QSize(1024, 768);
        default:
            Q_ASSERT(false);
    }
}

QSharedPointer<QOpenGLTexture> RenderContext::noiseTexture(int chain) {
    return m_noiseTextures.at(chain);
}

QSharedPointer<QOpenGLTexture> RenderContext::blankTexture() {
    return m_blankTexture;
}

void RenderContext::render(Model *model, int chain) {
    qDebug() << "RENDER!" << model << chain;
    qDebug() << model->graph().vertices();
}

void RenderContext::addRenderTrigger(QQuickWindow *window, Model *model, int chain) {
    RenderTrigger rt(this, model, chain, window);
    if(m_renderTriggers.contains(rt)) return;
    m_renderTriggers.append(rt);
    connect(window, &QQuickWindow::beforeSynchronizing, &m_renderTriggers.back(), &RenderTrigger::render, Qt::DirectConnection);
}

void RenderContext::removeRenderTrigger(QQuickWindow *window, Model *model, int chain) {
    RenderTrigger rt(this, model, chain, window);
    m_renderTriggers.removeAll(rt);
}

// RenderTrigger methods

RenderTrigger::RenderTrigger(RenderContext *context, Model *model, int chain, QObject *obj)
        : m_context(context)
        , m_model(model)
        , m_chain(chain)
        , m_obj(obj) {
}

RenderTrigger::~RenderTrigger() {
}

void RenderTrigger::render() {
    m_context->render(m_model, m_chain);
}

bool RenderTrigger::operator==(const RenderTrigger &other) const {
    return m_context == other.m_context
        && m_model == other.m_model
        && m_chain == other.m_chain
        && m_obj == other.m_obj;
}

RenderTrigger::RenderTrigger(const RenderTrigger& other)
    : m_context(other.m_context)
    , m_model(other.m_model)
    , m_chain(other.m_chain)
    , m_obj(other.m_obj) {
}
