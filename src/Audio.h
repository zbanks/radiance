#pragma once

#include <QObject>
#include <QThread>
#include <QVector>
#include <QAtomicInt>
#include <QMutex>

class Audio : public QThread {
    Q_OBJECT

public:
    Audio(QObject *p = nullptr);
   ~Audio() override;
    double time();

protected:
    void run();
    QMutex m_audioLock;

private:
    QVector<float> m_chunk;
    QAtomicInt m_run;
    double m_time;

public slots:
    void quit();
};
