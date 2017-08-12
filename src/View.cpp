#include "View.h"

View::View()
    : m_model(nullptr)
{
}

View::~View() {
}

void View::rebuild() {
    m_children.clear();
    if(m_model == nullptr) return;
    //for() {
    //}
}

Model *View::model() {
    return m_model;
}

void View::setModel(Model *model) {
    if(m_model != nullptr) disconnect(model, nullptr, this, nullptr);
    m_model = model;
    // Connect signals from model to view here
    rebuild();
    emit modelChanged(model);
}
