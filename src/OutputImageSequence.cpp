#include "OutputImageSequence.h"
#include <QTimer>

class OutputImageSequenceWorker : public OpenGLWorker {
    Q_OBJECT

public:
    OutputImageSequenceWorker(OutputImageSequence *p, int frameDelay, QString filename)
        : m_p(p)
        , OpenGLWorker(p->m_context)
        , m_filename(filename)
        , m_frameDelay(frameDelay) {
    }

    void display(GLuint textureId) {
        qDebug() << "Display" << textureId;
    }

public slots:
    void start() {
        m_timer = QSharedPointer<QTimer>(new QTimer(this));
        m_timer->setInterval(m_frameDelay);
        connect(m_timer.data(), &QTimer::timeout, this, &OutputImageSequenceWorker::onTimeout);
        m_timer->start();
    }

    void stop() {
        m_timer->stop();
        emit stopped();
    }

protected slots:
    void onTimeout() {
        makeCurrent();
        m_p->requestRender();
    }

signals:
    void stopped();

protected:
    OutputImageSequence *m_p;
    QString m_filename;
    int m_frameDelay;
    QSharedPointer<QTimer> m_timer;
};

OutputImageSequence::OutputImageSequence()
    : m_frameDelay(16)
    , m_size(QSize(300, 300))
    , m_filename("output_image_sequence")
    , m_enabled(false)
    , m_context(new OpenGLWorkerContext()) {
    m_context->thread()->start();
    m_context->thread()->setObjectName("OutputImageSequenceOpenGLWorkerContextThread");

    connect(this, &QObject::destroyed, this, &OutputImageSequence::onDestroyed);
    setChain(QSharedPointer<Chain>(new Chain(m_size)));
}

OutputImageSequence::~OutputImageSequence() {
}

void OutputImageSequence::setEnabled(bool enabled) {
    if (enabled && !m_enabled) {
        join();
        if (m_chain->size() != m_size) {
            setChain(QSharedPointer<Chain>(new Chain(m_size)));
        }
        m_worker = QSharedPointer<OutputImageSequenceWorker>(new OutputImageSequenceWorker(this, m_frameDelay, m_filename));
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
    if (!m_worker.isNull()) {
        QEventLoop loop;
        connect(m_worker.data(), &OutputImageSequenceWorker::stopped, &loop, &QEventLoop::quit);
        bool result = QMetaObject::invokeMethod(m_worker.data(), "stop");
        Q_ASSERT(result);
        loop.exec();
    }
}

bool OutputImageSequence::enabled() {
    return m_enabled;
}

void OutputImageSequence::onDestroyed() {
    join();
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
