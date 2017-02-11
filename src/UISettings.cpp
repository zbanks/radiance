#include "UISettings.h"
#include "main.h"

UISettings::UISettings()
    : m_previewSize(QSize(1, 1))
    , m_outputSize(QSize(1, 1)) {
}

QSize UISettings::previewSize() {
    return m_previewSize;
}

void UISettings::setPreviewSize(QSize value) {
    m_previewSize = value;
    emit previewSizeChanged(value);
}

QSize UISettings::outputSize() {
    return m_outputSize;
}

void UISettings::setOutputSize(QSize value) {
    m_outputSize = value;
    emit outputSizeChanged(value);
}
