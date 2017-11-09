#include "OutputImageSequence.h"
#include <QTimer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTextureBlitter>
#include <QDir>

class OutputImageSequenceWorker : public OpenGLWorker {
    Q_OBJECT

public:
    OutputImageSequenceWorker(OutputImageSequence *p, int frameDelay, QString filename, QSize size)
        : OpenGLWorker(p->m_context)
        , m_p(p)
        , m_filename(filename)
        , m_frameDelay(frameDelay)
        , m_size(size) {
    }

    void display(GLuint textureId) {
        m_fbo->bind();
        glClearColor(0, 0, 0, 1); // Replace with 0, 0, 0, 0 to get transparent images
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClear(GL_COLOR_BUFFER_BIT);

        if (textureId != 0) {
            m_blitter->bind();

            const QRect targetRect(QPoint(0, 0), m_fbo->size());
            const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(targetRect, targetRect);
            m_blitter->blit(textureId, target, QOpenGLTextureBlitter::OriginBottomLeft);

            m_blitter->release();
        }
        m_fbo->release();
        QString filename = m_filename + QString().sprintf("/%06d.png", m_index++);
        if (!m_fbo->toImage().save(filename)) {
            qWarning() << "Failed to write"  << filename;
        }
    }

public slots:
    void start() {
        makeCurrent();
        m_timer = QSharedPointer<QTimer>(new QTimer(this));
        m_fbo = QSharedPointer<QOpenGLFramebufferObject>(new QOpenGLFramebufferObject(m_size));
        m_blitter = QSharedPointer<QOpenGLTextureBlitter>(new QOpenGLTextureBlitter());
        m_blitter->create();

        QDir(m_filename).removeRecursively();
        if (!QDir().mkdir(m_filename)) {
            qWarning() << "Could not create directory" << m_filename;
            return;
        }
        m_index = 0;

        m_timer->setInterval(m_frameDelay);
        connect(m_timer.data(), &QTimer::timeout, this, &OutputImageSequenceWorker::onTimeout);
        m_timer->start();
    }

    void stop() {
        m_timer->stop();
        m_timer = QSharedPointer<QTimer>(nullptr);
        m_fbo = QSharedPointer<QOpenGLFramebufferObject>(nullptr);
        m_blitter = QSharedPointer<QOpenGLTextureBlitter>(nullptr);
    }

protected slots:
    void onTimeout() {
        makeCurrent();
        m_p->requestRender();
    }

protected:
    OutputImageSequence *m_p;
    QString m_filename;
    int m_frameDelay;
    QSize m_size;
    QSharedPointer<QTimer> m_timer;
    QSharedPointer<QOpenGLFramebufferObject> m_fbo;
    QSharedPointer<QOpenGLTextureBlitter> m_blitter;
    int m_index;
};

OutputImageSequence::OutputImageSequence()
    : m_size(QSize(300, 300))
    , m_frameDelay(16)
    , m_filename("output_image_sequence")
    , m_enabled(false)
    , m_context(OpenGLWorkerContext::create()) {
    m_context->thread()->start();
    m_context->thread()->setObjectName("OutputImageSequenceOpenGLWorkerContextThread");

    setChain(QSharedPointer<Chain>(new Chain(m_size)));
}

OutputImageSequence::~OutputImageSequence() {
    join();
//    m_context->deleteLater();
//    m_context->thread()->quit();
//    m_context->thread()->wait();
}

void OutputImageSequence::setEnabled(bool enabled) {
    if (enabled && !m_enabled) {
        join();
        if (m_chain->size() != m_size) {
            setChain(QSharedPointer<Chain>(new Chain(m_size)));
        }
        m_worker = QSharedPointer<OutputImageSequenceWorker>(new OutputImageSequenceWorker(this, m_frameDelay, m_filename, m_size),&QObject::deleteLater);
        bool result = QMetaObject::invokeMethod(m_worker.data(), "start");
        Q_ASSERT(result);

        m_enabled = enabled;
        emit enabledChanged(enabled);
    } else if (!enabled && m_enabled) {
        bool result = QMetaObject::invokeMethod(m_worker.data(), "stop");
        Q_ASSERT(result);

        m_enabled = enabled;
        emit enabledChanged(enabled);
    }
}

void OutputImageSequence::join() {
    if (m_worker) {
        bool result = QMetaObject::invokeMethod(m_worker.data(), "stop", Qt::BlockingQueuedConnection);
        Q_ASSERT(result);
        m_worker.reset();
    }
}

bool OutputImageSequence::enabled() {
    return m_enabled;
}

QSize OutputImageSequence::size() {
    return m_size;
}

void OutputImageSequence::setSize(QSize size) {
    if (size != m_size) {
        m_size = size;
        emit sizeChanged(size);
    }
}

QString OutputImageSequence::filename() {
    return m_filename;
}

void OutputImageSequence::setFilename(QString filename) {
    if (filename != m_filename) {
        m_filename = filename;
        emit filenameChanged(filename);
    }
}

qreal OutputImageSequence::fps() {
    return 1000. / m_frameDelay;
}

void OutputImageSequence::setFps(qreal fps) {
    int newDelay = 1000 / fps;
    if (newDelay != m_frameDelay) {
        m_frameDelay = newDelay;
        emit fpsChanged(1000. / newDelay);
    }
}

void OutputImageSequence::display(GLuint texture) {
    Q_ASSERT(!m_worker.isNull());
    m_worker->display(texture);
}

#include "OutputImageSequence.moc"
