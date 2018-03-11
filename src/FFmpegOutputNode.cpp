#include "FFmpegOutputNode.h"
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

FFmpegOutputNode::FFmpegOutputNode(Context *context, QSize chainSize)
    : SelfTimedReadBackOutputNode(context, chainSize, 1000 / 30)
    , m_recording(false)
{
    m_ffmpeg.setProgram("ffmpeg");
    m_ffmpeg.setStandardOutputFile(QProcess::nullDevice());
    m_ffmpeg.setProcessChannelMode(QProcess::MergedChannels);

    connect(this, &SelfTimedReadBackOutputNode::frame, this, &FFmpegOutputNode::onFrame, Qt::DirectConnection);
    connect(this, &SelfTimedReadBackOutputNode::initialize, this, &FFmpegOutputNode::onInitialize, Qt::DirectConnection);

    connect(&m_ffmpeg, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFFmpegFinished(int, QProcess::ExitStatus)));
}

FFmpegOutputNode::FFmpegOutputNode(const FFmpegOutputNode &other)
    : SelfTimedReadBackOutputNode(other)
    , m_recording(false)
    , m_ffmpegArguments(other.m_ffmpegArguments)
{
}

FFmpegOutputNode::~FFmpegOutputNode() {
    setRecording(false);
    m_ffmpeg.waitForFinished(100); // 100 ms
    m_ffmpeg.kill();
}

void FFmpegOutputNode::setRecording(bool recording) {
    qInfo() << "status" << m_ffmpeg.state() << m_ffmpeg.exitCode();
    if (m_recording == recording)
        return;

    if (recording) {
        m_recording = recording;
        {
            QMutexLocker locker(&m_ffmpegLock);
            qInfo() << "starting" << m_ffmpeg.program() << m_ffmpeg.arguments().join(" ");
            m_ffmpeg.start(QIODevice::WriteOnly | QIODevice::Unbuffered);
        }
        start();
        emit recordingChanged(recording);
    } else {
        stop();
        {
            QMutexLocker locker(&m_ffmpegLock);
            m_ffmpeg.closeWriteChannel();
            //m_ffmpeg.waitForBytesWritten();
            qInfo() << "ffmpeg closed..." << m_ffmpeg.bytesToWrite();
            // Callback to onFFmpegFinished when m_ffmpeg terminates
        }
    }
}

void FFmpegOutputNode::onFFmpegFinished(int exitCode, QProcess::ExitStatus status) {
    Q_ASSERT(QThread::currentThread() == thread());
    qInfo() << "ffmpeg finished";
    m_recording = false;
    emit recordingChanged(false);
}

bool FFmpegOutputNode::recording() {
    return m_recording;
}

QStringList FFmpegOutputNode::ffmpegArguments() {
    return m_ffmpegArguments;
}

void FFmpegOutputNode::setFFmpegArguments(QStringList ffmpegArguments) {
    setRecording(false);
    m_ffmpegArguments = ffmpegArguments;

    if (!ffmpegArguments.isEmpty()) {
        QSize size = chain().size();
        QString sizeStr(QString("%1x%2").arg(
                    QString::number(size.width()),
                    QString::number(size.height())));

        QMutexLocker locker(&m_ffmpegLock);
        m_ffmpeg.setArguments(QStringList() 
                << "-y"
                << "-vcodec" << "rawvideo"
                << "-f" << "rawvideo"
                << "-pix_fmt" << "rgba"
                << "-s" << sizeStr
                << "-i" << "pipe:0"
                << "-vf" << "vflip"
                << m_ffmpegArguments);
    }

    emit ffmpegArgumentsChanged(ffmpegArguments);
}

QString FFmpegOutputNode::typeName() {
    return "FFmpegOutputNode";
}

void FFmpegOutputNode::onInitialize() {
    // This is called from the STRBON worker thread
}

void FFmpegOutputNode::onFrame(QSize size, QByteArray frame) {
    // This is called from the STRBON worker thread
    QMutexLocker locker(&m_ffmpegLock);
    if (m_ffmpeg.state() == QProcess::NotRunning)
        return;
    m_ffmpeg.write(frame);
    qInfo() << "ffmpeg frame" << m_ffmpeg.bytesToWrite();
    m_ffmpeg.waitForBytesWritten();
}

VideoNode *FFmpegOutputNode::deserialize(Context *context, QJsonObject obj) {
    // TODO: You should be able to change the size of an OutputNode after
    // it has been created. For now this is hard-coded
    FFmpegOutputNode *e = new FFmpegOutputNode(context, QSize(128, 128));
    e->setFFmpegArguments({"-vcodec", "h264", "output.mp4"});
    return e;
}

bool FFmpegOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *FFmpegOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> FFmpegOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("FFmpegOutput", "FFmpegOutputInstantiator.qml");
    return m;
}
