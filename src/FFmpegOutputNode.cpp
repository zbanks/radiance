#include "FFmpegOutputNode.h"
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

FFmpegOutputNode::FFmpegOutputNode(Context *context, QSize chainSize)
    : OutputNode(new FFmpegOutputNodePrivate(context, chainSize))
{
}

FFmpegOutputNode::FFmpegOutputNode(const FFmpegOutputNode &other)
    : OutputNode(other)
{
}

FFmpegOutputNode *FFmpegOutputNode::clone() const {
    return new FFmpegOutputNode(*this);
}

QSharedPointer<FFmpegOutputNodePrivate> FFmpegOutputNode::d() const {
    return d_ptr.staticCast<FFmpegOutputNodePrivate>();
}

FFmpegOutputNode::~FFmpegOutputNode() {
    setRecording(false);
}

void FFmpegOutputNode::setRecording(bool recording) {
    QMutexLocker locker(&d()->m_stateLock);
    {
        if (d()->m_recording == recording)
            return;

        d()->m_recording = recording;

        if (recording) {
            QSize size = chain().size();
            QString sizeStr(QString("%1x%2").arg(
                        QString::number(size.width()),
                        QString::number(size.height())));

            d()->m_pixelBuffer.resize(3 * size.width() * size.height());

            d()->m_ffmpeg.start("ffmpeg", QStringList() 
                    << "-y"
                    << "-vcodec" << "rawvideo"
                    << "-f" << "rawvideo"
                    << "-pix_fmt" << "rgb24"
                    << "-s" << sizeStr
                    << "-i" << "pipe:0"
                    << "-vf" << "vflip"
                    << d()->m_ffmpegArguments);
        } else {
            d()->m_ffmpeg.closeWriteChannel();
            d()->m_ffmpeg.waitForFinished();
            //qInfo() << m_ffmpeg.readAllStandardOutput();
            //qInfo() << m_ffmpeg.readAllStandardError();
        }
    }
    emit recordingChanged(recording);
}

bool FFmpegOutputNode::recording() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_recording;
}

QStringList FFmpegOutputNode::ffmpegArguments() {
    QMutexLocker locker(&d()->m_stateLock);
    return d()->m_ffmpegArguments;
}

void FFmpegOutputNode::setFFmpegArguments(QStringList ffmpegArguments) {
    setRecording(false);

    {
        QMutexLocker locker(&d()->m_stateLock);
        d()->m_ffmpegArguments = ffmpegArguments;
    }
    emit ffmpegArgumentsChanged(ffmpegArguments);
}

QString FFmpegOutputNode::typeName() {
    return "FFmpegOutputNode";
}

VideoNode *FFmpegOutputNode::deserialize(Context *context, QJsonObject obj) {
    // TODO: You should be able to change the size of an OutputNode after
    // it has been created. For now this is hard-coded
    FFmpegOutputNode *e = new FFmpegOutputNode(context, QSize(128, 128));
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

void FFmpegOutputNode::recordFrame() {
    GLuint texture = render();
    {
        QMutexLocker locker(&d()->m_stateLock);
        if (!d()->m_recording)
            return;
        glBindTexture(GL_TEXTURE_2D, texture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, d()->m_pixelBuffer.data()); // XXX UNSAFE
        d()->m_ffmpeg.write(d()->m_pixelBuffer);
    }
}

FFmpegOutputNodePrivate::FFmpegOutputNodePrivate(Context *context, QSize chainSize)
    : OutputNodePrivate(context, chainSize)
    , m_recording(false)
    , m_ffmpegArguments({"-vcodec", "h264", "output.mp4"})
{
}
