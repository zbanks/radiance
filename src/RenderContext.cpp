#include "main.h"
#include "RenderContext.h"
#include "Model.h"
#include <memory>

RenderContext::RenderContext()
    : m_openGLWorker(this)
    , m_initialized(false)
    , m_blankTexture(QOpenGLTexture::Target2D) {
    connect(&m_openGLWorker, &RenderContextOpenGLWorker::initialized, this, &RenderContext::onInitialized);

    connect(&m_periodic, &QTimer::timeout, this, &RenderContext::periodic);
    m_periodic.start(10); // TODO make this adjustable?

    if (!QMetaObject::invokeMethod(&m_openGLWorker, "initialize"))
        qFatal("Unable to initialize openGLWorker");
}

RenderContext::~RenderContext() {
}

void RenderContext::onInitialized() {
    m_initialized = true;
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

GLuint RenderContext::noiseTexture(int chain) {
    return m_noiseTextures.at(chain)->textureId();
}

GLuint RenderContext::blankTexture() {
    return m_blankTexture.textureId();
}

void RenderContext::render(Model *model, int chain) {
    //qDebug() << "RENDER!" << model << chain;

    auto m = model->createCopyForRendering();

    // inputs is parallel to vertices
    // and contains the VideoNodes connected to the
    // corresponding vertex's inputs
    QVector<QVector<int>> inputs;

    // Create a list of -1's
    for (int i=0; i<m.vertices.count(); i++) {
        auto inputCount = m.vertices.at(i)->inputCount();
        inputs.append(QVector<int>(inputCount, -1));
    }

    Q_ASSERT(m.fromVertex.count() == m.toVertex.count());
    Q_ASSERT(m.fromVertex.count() == m.toInput.count());
    for (int i = 0; i < m.toVertex.count(); i++) {
        auto to = m.toVertex.at(i);
        if (to >= 0) {
            inputs[to][m.toInput.at(i)] = m.fromVertex.at(i);
        }
    }

    for (int i=0; i<m.vertices.count(); i++) {
        auto vertex = m.vertices.at(i);
        QVector<GLuint> inputTextures(vertex->inputCount(), blankTexture());
        for (int j=0; j<vertex->inputCount(); j++) {
            auto fromVertex = inputs.at(i).at(j);
            if (fromVertex != -1) {
                auto inpTexture = m.vertices.at(fromVertex)->texture(chain);
                if (inpTexture != 0) {
                    inputTextures[j] = inpTexture;
                }
            }
        }
        vertex->paint(chain, inputTextures);
        //qDebug() << vertex << "wrote texture" << vertex->texture(chain);
    }

    model->copyBackRenderStates(chain, m.origVertices, m.vertices);
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

// RenderContextOpenGLWorker methods

RenderContextOpenGLWorker::RenderContextOpenGLWorker(RenderContext *p)
    : OpenGLWorker(openGLWorkerContext)
    , m_p(p) {
}

void RenderContextOpenGLWorker::initialize() {
    makeCurrent();
    createBlankTexture();
    createNoiseTextures();
    emit initialized();
}

void RenderContextOpenGLWorker::createNoiseTextures() {
    Q_ASSERT(!m_p->m_initialized); // Make sure we are not "live"

    m_p->m_noiseTextures.clear();
    for(int i=0; i<m_p->chainCount(); i++) {
        auto tex = QSharedPointer<QOpenGLTexture>(new QOpenGLTexture(QOpenGLTexture::Target2D));
        tex->setSize(m_p->chainSize(i).width(), m_p->chainSize(i).height());
        tex->setFormat(QOpenGLTexture::RGBA8_UNorm);
        tex->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
        tex->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        tex->setWrapMode(QOpenGLTexture::Repeat);

        auto byteCount = m_p->chainSize(i).width() * m_p->chainSize(i).height() * 4;
        auto data = std::make_unique<uint8_t[]>(byteCount);
        qsrand(1);
        std::generate(&data[0],&data[0] + byteCount,qrand);
        tex->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
        m_p->m_noiseTextures.append(tex);
    }
}

void RenderContextOpenGLWorker::createBlankTexture() {
    Q_ASSERT(!m_p->m_initialized); // Make sure we are not "live"

    m_p->m_blankTexture.setSize(1, 1);
    m_p->m_blankTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_p->m_blankTexture.allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);
    m_p->m_blankTexture.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    m_p->m_blankTexture.setWrapMode(QOpenGLTexture::Repeat);

    auto data = std::make_unique<uint8_t[]>(4);
    m_p->m_blankTexture.setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, &data[0]);
}

