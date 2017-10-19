#pragma once
#include <QObject>

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

    Controls(QObject *parent=nullptr);
   ~Controls();

public slots:
    void changeControlAbs(int bank, Control control, qreal value);
    void changeControlRel(int bank, Control control, qreal value);

signals:
    void controlChangedAbs(int bank, Control control, qreal value);
    void controlChangedRel(int bank, Control control, qreal value);
};
