#include "QQuickOutputItem.h"
#include <QQuickWindow>
#include <QSGImageNode>

class QQuickOutputItemOutput : public Output {
    Q_OBJECT

public:
    QQuickOutputItemOutput(QQuickOutputItem *p)
        : m_p(p) {
    }

    void display(GLuint textureId) override {
        m_p->m_textureId = textureId;
    }

protected:
    QQuickOutputItem *m_p;
};

QQuickOutputItem::QQuickOutputItem()
    : m_output(QSharedPointer<QQuickOutputItemOutput>(new QQuickOutputItemOutput(this)))
    , m_size(QSize(300, 300))
    , m_textureId(0)
    , m_window(nullptr) {
    setFlags(QQuickItem::ItemHasContents);
    connect(this, &QQuickItem::windowChanged, this, &QQuickOutputItem::onWindowChanged);
    onWindowChanged(window());
    updateChain();
}

QQuickOutputItem::~QQuickOutputItem() {
}

void QQuickOutputItem::onWindowChanged(QQuickWindow *myWindow) {
    if(m_window ) {
        disconnect(m_window, &QQuickWindow::frameSwapped, this, &QQuickItem::update);
        disconnect(m_window, &QQuickWindow::beforeSynchronizing, m_output.data(), &Output::requestRender);
    }
    if(myWindow ) {
        connect(myWindow, &QQuickWindow::frameSwapped, this, &QQuickItem::update);
        connect(myWindow, &QQuickWindow::beforeSynchronizing, m_output.data(), &Output::requestRender, Qt::DirectConnection);
    }
    m_window = myWindow;
}

void QQuickOutputItem::updateChain() {
    m_output->setChain(QSharedPointer<Chain>(new Chain(m_size)));
}

Output *QQuickOutputItem::output() {
    return m_output.data();
}

QSize QQuickOutputItem::size() {
    return m_size;
}

void QQuickOutputItem::setSize(QSize size) {
    if (size != m_size) {
        m_size = size;
        updateChain();
        emit sizeChanged(size);
    }
}

QSGNode *QQuickOutputItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);

    if (!node) {
        node = window()->createImageNode();
        node->setFiltering(QSGTexture::Linear);
        node->setTextureCoordinatesTransform(QSGImageNode::MirrorVertically);
        node->setRect(boundingRect());
        node->setOwnsTexture(true);

        // Create a garbage QSGTexture if the real one is not ready yet
        node->setTexture(window()->createTextureFromId(0, QSize(1, 1), QQuickWindow::TextureHasAlphaChannel));
        // It is important that we generate a node even if we are not ready
        // so that we can mark it dirty. If we don't mark it dirty on the first call,
        // this function will never get called again
    }

    if (m_textureId != 0) {
        auto tex = node->texture();
        if(!tex || tex->textureId() != (int)m_textureId) {
            tex = window()->createTextureFromId(m_textureId, m_size, QQuickWindow::TextureHasAlphaChannel);
            tex->setFiltering(QSGTexture::Linear);
            node->setTexture(tex);
            // TODO repeatedly creating the QSGTexture is probably not the most efficient
            node->setRect(boundingRect());
        }
    }
    node->markDirty(QSGNode::DirtyMaterial); // Notifies all connected renderers that the node has dirty bits ;)
    return node;
}

#include "QQuickOutputItem.moc"
