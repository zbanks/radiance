#include "UISettings.h"
#include "main.h"

UISettings::UISettings() : m_previewSize(QSize(1, 1)) {
}

QSize UISettings::previewSize() {
    return m_previewSize;
}

void UISettings::setPreviewSize(QSize value) {
    m_previewSize = value;
    emit previewSizeChanged(value);
}
