#pragma once

#include <QAbstractItemModel>

class Registry;

class LibraryItem
{
public:
    explicit LibraryItem(const QString &name, const QString &fileToInstantiate, LibraryItem *parentItem = 0);
    ~LibraryItem();

    void appendChild(LibraryItem *child);

    LibraryItem *child(int row);
    int childCount() const;
    QString name() const;
    QString file() const;
    int row() const;
    LibraryItem *parentItem();

private:
    QList<LibraryItem*> m_childItems;
    LibraryItem *m_parentItem;
    QString m_name;
    QString m_file;
};

class Library : public QAbstractItemModel {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged);

public:
    Library(Registry *registry);
    ~Library();

    enum LibraryRoles {
        FileRole = Qt::UserRole + 1,
    };
    Q_ENUMS(LibraryRoles)

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool checkAgainstFilter(QString name);

public slots:
    QString filter();
    void setFilter(QString filter);

signals:
    void filterChanged(QString filter);

protected:
    void rebuild();
    void populate(LibraryItem *item, QString currentDirectory);
    LibraryItem *itemFromFile(QString filename, LibraryItem *parent);
    LibraryItem *m_rootItem;
    Registry *m_registry;
    QString m_filter;
};
