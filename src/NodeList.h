#pragma once

#include <QObject>

class NodeList : public QObject {
    Q_OBJECT

public slots:
    QStringList effectNames();
};

