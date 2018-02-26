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

FFmpegOutputNode::FFmpegOutputNode(const FFmpegOutputNode &other)
    : OutputNode(other) {
}

FFmpegOutputNode::~FFmpegOutputNode() {
    setRecording(false);
}

void FFmpegOutputNode::setRecording(bool recording) {
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

bool FFmpegOutputNode::recording() {
    return m_recording;
}

QStringList FFmpegOutputNode::ffmpegArguments() {
    return m_ffmpegArguments;
}

void FFmpegOutputNode::setFFmpegArguments(QStringList ffmpegArguments) {
    setRecording(false);
    m_ffmpegArguments = ffmpegArguments;
    emit ffmpegArgumentsChanged(m_ffmpegArguments);
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
    if (!m_recording)
        return;

    GLuint texture = render();
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pixelBuffer.data());
    m_ffmpeg.write(m_pixelBuffer);
}

QImage FFmpegOutputNode::grabImage() {
    QImage outputImage(chain()->size(), QImage::Format_RGBA8888_Premultiplied);

    GLuint texture = render();
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, outputImage.bits());

    return outputImage.mirrored(false, true); // vertical flip
}
