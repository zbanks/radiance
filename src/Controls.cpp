#include "Controls.h"

ControlsAttachedType::ControlsAttachedType(QObject *parent)
    : QObject(parent)
{   
}

ControlsAttachedType::~ControlsAttachedType() {
}

void ControlsAttachedType::changeControlAbs(int bank, Controls::Control control, qreal value) {
    emit controlChangedAbs(bank, control, value);
}

void ControlsAttachedType::changeControlRel(int bank, Controls::Control control, qreal value) {
    emit controlChangedRel(bank, control, value);
}

ControlsAttachedType *Controls::qmlAttachedProperties(QObject *object) {
    return new ControlsAttachedType(object);
}
