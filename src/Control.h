#pragma once
#include <QObject>

class Control : public QObject {
    Q_OBJECT

public:
    enum ControlEnum {
        None = 0,
        Enter,
        Cancel,
        Scroll,
        ScrollVertical,
        ScrollHorizontal,
        PrimaryParameter,
        SecondaryParameter,
    };

    Q_ENUM(ControlEnum)
};
