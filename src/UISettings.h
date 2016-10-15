#ifndef __UI_SETTINGS_H
#define __UI_SETTINGS_H

#include <QObject>
#include <QSize>
#include <QQmlEngine>
#include <QJSEngine>

class UISettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QSize previewSize READ previewSize WRITE setPreviewSize NOTIFY previewSizeChanged)

public:
    UISettings();
    QSize previewSize();
    void setPreviewSize(QSize value);

signals:
    void previewSizeChanged(QSize value);

private:
    QSize m_previewSize;
};

#endif
