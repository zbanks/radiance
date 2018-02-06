#pragma once

#include <QAbstractItemModel>

class LibraryItem
{
public:
    explicit LibraryItem(const QString &name, LibraryItem *parentItem = 0);
    ~LibraryItem();

    void appendChild(LibraryItem *child);

    LibraryItem *child(int row);
    int childCount() const;
    QString name() const;
    int row() const;
    LibraryItem *parentItem();

private:
    QList<LibraryItem*> m_childItems;
    LibraryItem *m_parentItem;
    QString m_name;
};

class Library : public QAbstractItemModel {
    Q_OBJECT

public:
    Library(QObject *parent = nullptr);
    ~Library();

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

protected:
    void rebuild();
    LibraryItem *m_rootItem;
};
