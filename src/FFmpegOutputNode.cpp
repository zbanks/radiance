#include "FFmpegOutputNode.h"
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

FFmpegOutputNode::FFmpegOutputNode(Context *context, QSize chainSize)
    : OutputNode(context, chainSize)
    , m_recording(false)
    , m_ffmpegArguments({"-vcodec", "h264", "output.mp4"})
{
}

FFmpegOutputNode::~FFmpegOutputNode() {
    setRecording(false);
}

void FFmpegOutputNode::setRecording(bool recording) {
    QMutexLocker locker(&m_stateLock);
    {
        if (m_recording == recording)
            return;

        m_recording = recording;

        if (recording) {
            QSize size = chain()->size();
            QString sizeStr(QString("%1x%2").arg(
                        QString::number(size.width()),
                        QString::number(size.height())));

            m_pixelBuffer.resize(3 * size.width() * size.height());

            m_ffmpeg.start("ffmpeg", QStringList() 
                    << "-y"
                    << "-vcodec" << "rawvideo"
                    << "-f" << "rawvideo"
                    << "-pix_fmt" << "rgb24"
                    << "-s" << sizeStr
                    << "-i" << "pipe:0"
                    << "-vf" << "vflip"
                    << m_ffmpegArguments);
        } else {
            m_ffmpeg.closeWriteChannel();
            m_ffmpeg.waitForFinished();
            //qInfo() << m_ffmpeg.readAllStandardOutput();
            //qInfo() << m_ffmpeg.readAllStandardError();
        }
    }
    emit recordingChanged(recording);
}

bool FFmpegOutputNode::recording() {
    QMutexLocker locker(&m_stateLock);
    return m_recording;
}

QStringList FFmpegOutputNode::ffmpegArguments() {
    QMutexLocker locker(&m_stateLock);
    return m_ffmpegArguments;
}

void FFmpegOutputNode::setFFmpegArguments(QStringList ffmpegArguments) {
    setRecording(false);

    {
        QMutexLocker locker(&m_stateLock);
        m_ffmpegArguments = ffmpegArguments;
    }
    emit ffmpegArgumentsChanged(ffmpegArguments);
}

QString FFmpegOutputNode::typeName() {
    return "FFmpegOutputNode";
}

VideoNodeSP *FFmpegOutputNode::deserialize(Context *context, QJsonObject obj) {
    // TODO: You should be able to change the size of an OutputNode after
    // it has been created. For now this is hard-coded
    return new FFmpegOutputNodeSP(new FFmpegOutputNode(context, QSize(128, 128)));
}

bool FFmpegOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNodeSP *FFmpegOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> FFmpegOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("FFmpegOutput", "FFmpegOutputInstantiator.qml");
    return m;
}

void FFmpegOutputNode::recordFrame() {
    GLuint texture = render();
    {
        QMutexLocker locker(&m_stateLock);
        if (!m_recording)
            return;
        glBindTexture(GL_TEXTURE_2D, texture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pixelBuffer.data()); // XXX UNSAFE
        m_ffmpeg.write(m_pixelBuffer);
    }
}
