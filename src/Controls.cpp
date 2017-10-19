#include "Controls.h"

void Controls::changeControlAbs(int bank, Controls::Control control, qreal value) {
    emit controlChangedAbs(bank, control, value);
}

void Controls::changeControlRel(int bank, Controls::Control control, qreal value) {
    emit controlChangedRel(bank, control, value);
}

Controls::Controls(QObject *parent)
    : QObject(parent) {
}

Controls::~Controls() {
}
