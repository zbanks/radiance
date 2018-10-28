#pragma once
#include <QQuickTextDocument>

class GlslDocument : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY messageChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)

public slots:
    bool load(QString filename);
    bool save(QString filename);
    void clear();
    QQuickTextDocument *document();
    void setDocument(QQuickTextDocument *document);
    QString message();
    void setMessage(QString message);
    bool modified();
    bool revert(QString filename);
    QString loadDirectory(QString filename);
    QString saveDirectory(QString filename);
    QString contractLibraryPath(QString filename);
    int cursorPositionAt(int line, int col);

signals:
    void documentChanged(QQuickTextDocument *document);
    void messageChanged(QString message);
    void modifiedChanged(bool modified);

protected:
    QQuickTextDocument *m_document{};
    QString m_message;

protected slots:
    void onContentsChanged();
};
