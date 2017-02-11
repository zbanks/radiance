#ifndef __UI_SETTINGS_H
#define __UI_SETTINGS_H

#include <QObject>
#include <QSize>
#include <QQmlEngine>
#include <QJSEngine>

class UISettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QSize previewSize READ previewSize WRITE setPreviewSize NOTIFY previewSizeChanged)
    Q_PROPERTY(QSize outputSize READ outputSize WRITE setOutputSize NOTIFY outputSizeChanged)

public:
    UISettings();
    QSize previewSize();
    void setPreviewSize(QSize value);
    QSize outputSize();
    void setOutputSize(QSize value);

signals:
    void previewSizeChanged(QSize value);
    void outputSizeChanged(QSize value);

private:
    QSize m_previewSize;
    QSize m_outputSize;
};

#endif
