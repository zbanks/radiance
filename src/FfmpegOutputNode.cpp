#include "FfmpegOutputNode.h"
#include <QDebug>
#include <QJsonObject>
#include <QGuiApplication>

FfmpegOutputNode::FfmpegOutputNode(Context *context, QSize chainSize)
    : OutputNode(context, chainSize)
    , m_recording(false)
    , m_ffmpegArguments({"-vcodec", "h264", "output.mp4"})
{

}

FfmpegOutputNode::FfmpegOutputNode(const FfmpegOutputNode &other)
    : OutputNode(other) {
}

FfmpegOutputNode::~FfmpegOutputNode() {
    setRecording(false);
}

void FfmpegOutputNode::setRecording(bool recording) {
    if (m_recording == recording)
        return;

    m_recording = recording;
    if (m_recording) {
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
    emit recordingChanged(m_recording);
}

bool FfmpegOutputNode::recording() {
    return m_recording;
}

QStringList FfmpegOutputNode::ffmpegArguments() {
    return m_ffmpegArguments;
}

void FfmpegOutputNode::setFfmpegArguments(QStringList ffmpegArguments) {
    m_ffmpegArguments = ffmpegArguments;
    emit ffmpegArgumentsChanged(m_ffmpegArguments);
}

QString FfmpegOutputNode::typeName() {
    return "FfmpegOutputNode";
}

VideoNode *FfmpegOutputNode::deserialize(Context *context, QJsonObject obj) {
    FfmpegOutputNode *e = new FfmpegOutputNode(context, QSize(128, 128));
    return e;
}

bool FfmpegOutputNode::canCreateFromFile(QString filename) {
    return false;
}

VideoNode *FfmpegOutputNode::fromFile(Context *context, QString filename) {
    return nullptr;
}

QMap<QString, QString> FfmpegOutputNode::customInstantiators() {
    auto m = QMap<QString, QString>();
    m.insert("FfmpegOutput", "FfmpegOutputInstantiator.qml");
    return m;
}

void FfmpegOutputNode::recordFrame() {
    if (!m_recording)
        return;

    GLuint texture = render();
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pixelBuffer.data());
    m_ffmpeg.write(m_pixelBuffer);
}
