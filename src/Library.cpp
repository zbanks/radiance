#include "Library.h"
#include "Paths.h"
#include "Registry.h"
#include <QDir>
#include <QDebug>
#include <QtGlobal>

LibraryItem::LibraryItem(const QString &name, const QString &fileToInstantiate, LibraryItem *parent)
{
    m_parentItem = parent;
    m_name = name;
    m_file = fileToInstantiate;
}

LibraryItem::~LibraryItem()
{
    qDeleteAll(m_childItems);
}

void LibraryItem::appendChild(LibraryItem *item)
{
    m_childItems.append(item);
}

LibraryItem *LibraryItem::child(int row)
{
    return m_childItems.value(row);
}

int LibraryItem::childCount() const
{
    return m_childItems.count();
}

int LibraryItem::row() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<LibraryItem*>(this));
    }

    return 0;
}

QString LibraryItem::name() const
{
    return m_name;
}

QString LibraryItem::file() const
{
    return m_file;
}

LibraryItem *LibraryItem::parentItem()
{
    return m_parentItem;
}

Library::Library(Registry *registry)
    : QAbstractItemModel()
    , m_rootItem(nullptr)
    , m_registry(registry)
    , m_filter("") {

    rebuild();
}

Library::~Library() {
    delete m_rootItem;
}

QString Library::filter() {
    return m_filter;
}

void Library::setFilter(QString filter) {
    if (m_filter != filter) {
        m_filter = filter;
        rebuild();
        emit filterChanged(filter);
    }
}

bool Library::checkAgainstFilter(QString name) {
    if (m_filter.isEmpty()) return true;
    if (name.toLower().startsWith(m_filter.toLower())) return true;
    return false;
}

LibraryItem *Library::itemFromFile(QString filename, LibraryItem *parent) {
    if (!m_registry->canCreateFromFile(filename)) return nullptr;
    auto baseName = QFileInfo(filename).baseName();
    if (!checkAgainstFilter(baseName)) return nullptr;
    return new LibraryItem(baseName, filename, parent);
}

void Library::populate(LibraryItem *item, QString currentDirectory = ".") {
    QDir systemDir(Paths::systemLibrary() + "/" + currentDirectory);
    QDir userDir(Paths::userLibrary() + "/" + currentDirectory);
    auto systemLs = systemDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    auto userLs = userDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    auto lsList = systemLs + userLs;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    auto ls = lsList.toSet().toList();
#else
    auto ls = QSet<QString>(lsList.begin(), lsList.end()).values();
#endif
    ls.sort(Qt::CaseInsensitive);
    for (auto f = ls.begin(); f != ls.end(); f++) {
        auto path = currentDirectory + "/" + *f;
        if (QDir(Paths::systemLibrary() + "/" + path).exists() || QDir(Paths::userLibrary() + "/" + path).exists()) {
            auto newItem = new LibraryItem(*f, "", item);
            populate(newItem, path);
            // Add directories only if
            // they have contents,
            // or there is no filter set
            if (newItem->childCount() || m_filter.isEmpty()) {
                item->appendChild(newItem);
            } else {
                delete newItem;
            }
        } else {
            auto newItem = itemFromFile(path, item);
            if (newItem != nullptr) {
                item->appendChild(newItem);
            }
        }
    }
}

void Library::addCustomInstantiators(LibraryItem *item, QMap<QString, QString> instantiators) {
    auto customItem = new LibraryItem("Custom", "", item);
    for (auto e = instantiators.begin(); e != instantiators.end(); e++) {
        if (checkAgainstFilter(e.key())) {
            auto newItem = new LibraryItem(e.key(), e.value(), customItem);
            customItem->appendChild(newItem);
        }
    }
    if (customItem->childCount() || m_filter.isEmpty()) {
        item->appendChild(customItem);
    } else {
        delete customItem;
    }
}

void Library::rebuild() {
    beginResetModel();
    delete m_rootItem;
    m_rootItem = new LibraryItem("", "");
    populate(m_rootItem);
    addCustomInstantiators(m_rootItem, m_registry->instantiators());
    endResetModel();
}

QVariant Library::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    LibraryItem *item = static_cast<LibraryItem*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        return item->name();
    } else if (role == Library::FileRole) {
        return item->file();
    } else {
        return QVariant();
    }

}

QModelIndex Library::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    LibraryItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<LibraryItem*>(parent.internalPointer());

    LibraryItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex Library::parent(const QModelIndex &index) const {
    if (!index.isValid())
        return QModelIndex();

    LibraryItem *childItem = static_cast<LibraryItem*>(index.internalPointer());
    LibraryItem *parentItem = childItem->parentItem();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int Library::rowCount(const QModelIndex &parent) const {
    LibraryItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<LibraryItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int Library::columnCount(const QModelIndex &parent) const {
    return 1;
}

Qt::ItemFlags Library::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QHash<int, QByteArray> Library::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "name";
    roles[FileRole] = "file";
    return roles;
}
