#include "Model.h"
#include "main.h"

Model::Model()
    : m_context(renderContext) {
}

Model::~Model() {
}

VideoNode *Model::addVideoNode(QString type) {
}

void Model::removeVideoNode(VideoNode *videoNode) {
}

void Model::addEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
}

void Model::removeEdge(VideoNode *fromVertex, VideoNode *toVertex, int toInput) {
}
