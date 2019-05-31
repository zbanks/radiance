#pragma once
#include "BaseVideoNodeTile.h"
#include "Model.h"
#include "Controls.h"

struct Child {
    QSharedPointer<VideoNode> videoNode;
    BaseVideoNodeTile *item;
    QVector<int> inputHeights;
};

class View : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(ModelSP *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QVariantMap delegates READ qml_delegates WRITE qml_setDelegates NOTIFY qml_delegatesChanged)

public:
    View();
    ~View() override;

    ModelSP *model();
    void setModel(ModelSP *model);
    QMap<QString, QString> delegates();
    void setDelegates(QMap<QString, QString> delegates);
    QVariantMap qml_delegates();
    void qml_setDelegates(QVariantMap delegates);

public slots:
    void onGraphChanged();

    // Selection
    void select(QVariantList tiles);
    void addToSelection(QVariantList tiles);
    void removeFromSelection(QVariantList tiles);
    void toggleSelection(QVariantList tiles);
    void ensureSelected(BaseVideoNodeTile *tile);
    QVariantList selection();

    // Finds the connected components of the selection
    // Each connected component will have zero or more inputs and one output
    // (though possibly multiple output edges.)
    // This is useful because it may be treated as a single tile.
    // Returns a list of objects with three keys:
    // * tiles = A QVariantList of tiles contained within the connected component
    // * vertices = A QVariantList of vertices (VideoNodes) contained within the connected component
    // * edges = A QVariantList of edges contained within the connected component
    // * inputEdges = A QVariantList of input edges to the connected component (ordered)
    // * outputEdges = A QVariantList of output edges from the connected component (unordered)
    // * inputPorts = A QVariantList of QVariantMaps of {vertex, input}
    // * outputNode = The output VideoNode
    QVariantList selectedConnectedComponents();

    // Finds all tiles in between tile1 and tile2
    // Returns a QVariantList of tiles
    QVariantList tilesBetween(BaseVideoNodeTile *tile1, BaseVideoNodeTile *tile2);

    // Returns the tile for the given VideoNode instance
    BaseVideoNodeTile *tileForVideoNode(VideoNodeSP *videoNode);

    // The tile that has focus,
    // or nullptr if no tile has focus
    BaseVideoNodeTile *focusedChild();

protected:
    // m_model needs to be a ModelSP since there are QML properties that fetch it
    // This one ModelSP pointer can be shared by all of the UI and not cause problems
    ModelSP *m_model;
    QMap<QString, QString> m_delegates;
    QList<Child> m_children;
    QList<QQuickItem *> m_dropAreas;
    void rebuild();
    Child newChild(QSharedPointer<VideoNode> videoNode);
    QSet<BaseVideoNodeTile *> m_selection;
    void selectionChanged();
    void componentComplete() override;

protected slots:
    void onControlChangedAbs(int bank, Controls::Control control, qreal value);
    void onControlChangedRel(int bank, Controls::Control control, qreal value);

private:
    QQuickItem *createDropArea();

signals:
    void modelChanged(ModelSP *model);
    void qml_delegatesChanged(QVariantMap delegates);
    void delegatesChanged(QMap<QString, QString> delegates);
};
