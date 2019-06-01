#pragma once

#include <QQuickItem>
#include "Controls.h"
#include "QmlSharedPointer.h"

class Model;
typedef QmlSharedPointer<Model> ModelSP;
class VideoNode;
typedef QmlSharedPointer<VideoNode> VideoNodeSP;

class BaseVideoNodeTile : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(ModelSP *model MEMBER m_model NOTIFY onModelChanged);
    Q_PROPERTY(VideoNodeSP *videoNode MEMBER m_videoNode NOTIFY onVideoNodeChanged);
    Q_PROPERTY(QVariantList inputGridHeights MEMBER m_inputGridHeights NOTIFY onInputGridHeightsChanged);
    Q_PROPERTY(QVariantList inputHeights MEMBER m_inputHeights NOTIFY onInputHeightsChanged);
    Q_PROPERTY(int gridX MEMBER m_gridX NOTIFY onGridXChanged);
    Q_PROPERTY(int gridY MEMBER m_gridY NOTIFY onGridYChanged);
    Q_PROPERTY(qreal posX MEMBER m_posX NOTIFY onPosXChanged);
    Q_PROPERTY(qreal posY MEMBER m_posY NOTIFY onPosYChanged);
    Q_PROPERTY(QVariant tab MEMBER m_tab NOTIFY onTabChanged);
    Q_PROPERTY(QVariant backtab MEMBER m_backtab NOTIFY onBacktabChanged);
    Q_PROPERTY(qreal normalHeight MEMBER m_normalHeight NOTIFY onNormalHeightChanged)
    Q_PROPERTY(qreal normalWidth MEMBER m_normalWidth NOTIFY onNormalWidthChanged)
    Q_PROPERTY(qreal minInputHeight MEMBER m_minInputHeight NOTIFY onMinInputHeightChanged)
    Q_PROPERTY(qreal blockWidth MEMBER m_blockWidth NOTIFY onBlockWidthChanged)
    Q_PROPERTY(qreal blockHeight MEMBER m_blockHeight NOTIFY onBlockHeightChanged)
    Q_PROPERTY(int bank MEMBER m_bank NOTIFY onBankChanged)

public:
    BaseVideoNodeTile(QQuickItem *p = nullptr);
   ~BaseVideoNodeTile() override;

signals:
    void onModelChanged(ModelSP *model);
    void onVideoNodeChanged(VideoNodeSP *videoNode);
    void onInputGridHeightsChanged(QVariantList inputGridHeights);
    void onInputHeightsChanged(QVariantList inputHeights);
    void onGridXChanged(int gridX);
    void onGridYChanged(int gridY);
    void onPosXChanged(qreal posX);
    void onPosYChanged(qreal posY);
    void onTabChanged(QVariant tab);
    void onBacktabChanged(QVariant backtab);
    void onNormalHeightChanged(qreal normalHeight);
    void onNormalWidthChanged(qreal normalWidth);
    void onMinInputHeightChanged(qreal minInputHeight);
    void onBlockWidthChanged(qreal blockWidth);
    void onBlockHeightChanged(qreal blockHeight);
    void onBankChanged(int bank);

protected:
    ModelSP *m_model{};
    VideoNodeSP *m_videoNode{};

    qreal m_normalHeight{};
    qreal m_normalWidth{};
    qreal m_minInputHeight{}; // TODO bind to normalHeight
    qreal m_blockWidth{}; // TODO bind to normalWidth
    qreal m_blockHeight{}; // TODO bind to normalHeight
    int m_bank{};

    QVariantList m_inputGridHeights{};
    QVariantList m_inputHeights{};
    int m_gridX{};
    int m_gridY{};
    qreal m_posX{-1};
    qreal m_posY{-1};
    QVariant m_tab{};
    QVariant m_backtab{};
};
