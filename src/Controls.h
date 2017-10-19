#pragma once
#include <QObject>
#include <QQmlEngine>

class ControlsAttachedType;

class Controls : public QObject {
    Q_OBJECT

public:
    enum Control {
        None = 0,
        Enter,
        Cancel,
        Scroll,
        ScrollVertical,
        ScrollHorizontal,
        PrimaryParameter,
        SecondaryParameter,
    };
    Q_ENUM(Control)

    static ControlsAttachedType *qmlAttachedProperties(QObject *object);
};

QML_DECLARE_TYPEINFO(Controls, QML_HAS_ATTACHED_PROPERTIES)

class ControlsAttachedType : public QObject {
    Q_OBJECT

public:
    ControlsAttachedType(QObject *parent=nullptr);
   ~ControlsAttachedType();

public slots:
    void changeControlAbs(int bank, Controls::Control control, qreal value);
    void changeControlRel(int bank, Controls::Control control, qreal value);

signals:
    void controlChangedAbs(int bank, Controls::Control control, qreal value);
    void controlChangedRel(int bank, Controls::Control control, qreal value);
};
