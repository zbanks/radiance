#pragma once

#include <QSharedPointer>
#include <QMetaMethod>

template <class T>
class QmlSharedPointer
    : public QObject
    , public QSharedPointer<T>
{
// Q_OBJECT macro written out
public:
    QT_WARNING_PUSH
    Q_OBJECT_NO_OVERRIDE_WARNING
    static const QMetaObject staticMetaObject;
    QT_TR_FUNCTIONS
private:
    Q_OBJECT_NO_ATTRIBUTES_WARNING
    Q_DECL_HIDDEN_STATIC_METACALL static void qt_static_metacall(QObject *, QMetaObject::Call, int, void **);
    QT_WARNING_POP
    struct QPrivateSignal {};
    QT_ANNOTATE_CLASS(qt_qobject, "")
// end Q_OBJECT

private:
    void init()
    {
        // Connect all signals from the encapsulated item to the QmlSharedPointer
        // Skip QObject's signals (e.g. destroyed)
        const int qObjectMethodCount = QObject::staticMetaObject.methodCount();
        for (int i = qObjectMethodCount; i < T::staticMetaObject.methodCount(); i++)
        {
            auto incomingSignal = T::staticMetaObject.method(i);
            if (incomingSignal.methodType() != QMetaMethod::Signal) continue;
            QMetaObject::connect(QmlSharedPointer<T>::data(), i, this, i);
        }
    }
    void deinit()
    {
        // Disconnect all signals from the encapsulated item to the QmlSharedPointer
        // Skip QObject's signals (e.g. destroyed)
        const int qObjectMethodCount = QObject::staticMetaObject.methodCount();
        for (int i = qObjectMethodCount; i < T::staticMetaObject.methodCount(); i++)
        {
            auto incomingSignal = T::staticMetaObject.method(i);
            if (incomingSignal.methodType() != QMetaMethod::Signal) continue;
            QMetaObject::disconnect(QmlSharedPointer<T>::data(), i, this, i);
        }
    }
    static const QMetaObject *gen_superdata();
    static const QByteArrayData *gen_stringdata();
    static const uint *gen_data();
    static const QMetaObject * const *gen_relatedMetaObjects();
    static void *gen_extradata();

public:
    QmlSharedPointer()
        : QSharedPointer<T>(new T())
    {
        init();
    }

    template <typename X> explicit QmlSharedPointer(X *ptr)
        : QSharedPointer<T>(ptr)
    {
        init();
    }

    template <typename X, typename Deleter> QmlSharedPointer(X *ptr, Deleter d)
        : QSharedPointer<T>(ptr, d)
    {
        init();
    }

    QmlSharedPointer(const QSharedPointer<T> &other)
        : QSharedPointer<T>(other)
    {
        init();
    }

    QmlSharedPointer(const QmlSharedPointer<T> &other)
        : QSharedPointer<T>(other)
    {
        init();
    }

    QmlSharedPointer(const QWeakPointer<T> &other)
        : QSharedPointer<T>(other)
    {
        init();
    }

    QmlSharedPointer<T>& operator=(const QmlSharedPointer<T>& d)
    {
        deinit();
        QSharedPointer<T>::operator=(d);
        init();
        return *this;
    }

    // The MOC doesn't generate this for us anymore
    const QMetaObject *metaObject() const
    {
        return &staticMetaObject;
    }

    void *qt_metacast(const char *_clname)
    {
        //qDebug() << "Attempted metacast";
        return nullptr; // disable casting
    }

    int qt_metacall(QMetaObject::Call _c, int _id, void **_a)
    {
        const int n_methods = staticMetaObject.methodCount() - staticMetaObject.d.superdata->methodCount();
        const int n_properties = staticMetaObject.propertyCount() - staticMetaObject.d.superdata->propertyCount();
        _id = QObject::qt_metacall(_c, _id, _a);
        if (_id < 0)
            return _id;
        if (_c == QMetaObject::InvokeMetaMethod) {
            if (_id < n_methods)
                qt_static_metacall(this, _c, _id, _a);
            _id -= n_methods;
        } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
            if (_id < n_methods)
                *reinterpret_cast<int*>(_a[0]) = -1;
            _id -= n_methods;
        }
       else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
                || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
            qt_static_metacall(this, _c, _id, _a);
            _id -= n_properties;
        } else if (_c == QMetaObject::QueryPropertyDesignable) {
            _id -= n_properties;
        } else if (_c == QMetaObject::QueryPropertyScriptable) {
            _id -= n_properties;
        } else if (_c == QMetaObject::QueryPropertyStored) {
            _id -= n_properties;
        } else if (_c == QMetaObject::QueryPropertyEditable) {
            _id -= n_properties;
        } else if (_c == QMetaObject::QueryPropertyUser) {
            _id -= n_properties;
        }
        return _id;
    }
};

template <typename T> void QmlSharedPointer<T>::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QmlSharedPointer<T> *>(_o);
    auto *_child = _t->data();
    Q_ASSERT(_child != nullptr);

    if (_c == QMetaObject::InvokeMetaMethod) {
        _id += QObject::staticMetaObject.methodCount();

        auto method = QmlSharedPointer<T>::staticMetaObject.method(_id);
        auto sig = QMetaObject::normalizedSignature(method.methodSignature());
        if (method.methodType() == QMetaMethod::Signal) {
            auto newId = _id - QObject::staticMetaObject.methodCount();
            QMetaObject::activate(_o, &staticMetaObject, newId, _a);
        } else {
            _child->qt_metacall(_c, _id, _a);
        }
    } else if (_c == QMetaObject::IndexOfMethod ||
               _c == QMetaObject::ReadProperty || 
               _c == QMetaObject::WriteProperty ||
               _c == QMetaObject::ResetProperty) {
        _id += QObject::staticMetaObject.propertyCount();
        _child->qt_metacall(_c, _id, _a);
    } else {
        Q_ASSERT(false); // Unhandled request
    }
}

template<typename T>
const QMetaObject *QmlSharedPointer<T>::gen_superdata()
{
    return T::staticMetaObject.d.superdata;
}

template<typename T>
const QByteArrayData *QmlSharedPointer<T>::gen_stringdata()
{
    // The MOC always places the strings right after the QByteArrayDatas,
    // so we can back out the number of strings based on the first offset
    int n_strings = T::staticMetaObject.d.stringdata[0].offset / sizeof (T::staticMetaObject.d.stringdata[0]);

    // Copy the child's string data
    // (copy the lookup table, but don't copy the actual strings.
    // Just point the new lookup table at the old strings)
    const int MAX_N_STRINGS = 128;
    Q_ASSERT(n_strings <= MAX_N_STRINGS);
    static QByteArrayData new_stringdata[MAX_N_STRINGS];

    for (int i=0; i<MAX_N_STRINGS; i++) {
        new_stringdata[i].ref.atomic.store(-1); // Don't attempt to free
    }

    for (int i=0; i<n_strings; i++) {
        new_stringdata[i].size = T::staticMetaObject.d.stringdata[i].size;
        new_stringdata[i].offset = T::staticMetaObject.d.stringdata[i].offset + ((qptrdiff)&(T::staticMetaObject.d.stringdata[i]) - (qptrdiff)&new_stringdata[i]);
    }

    // Override the first string, which is the name:
    const int MAX_N_CHARS = 128;
    static char new_name[MAX_N_CHARS]{};
    strcpy(new_name, "QmlSharedPointer<");
    strncat(new_name, T::staticMetaObject.className(), 127);
    strncat(new_name, ">", 127);
    new_stringdata[0].size = strlen(new_name);
    new_stringdata[0].offset = ((qptrdiff)&new_name - (qptrdiff)&new_stringdata[0]);

    //const QByteArrayDataPtr first = { const_cast<QByteArrayData*>(&new_stringdata[0]) };
    //qDebug() << "new_stringdata[0]" << (QByteArray)first;
 
    return new_stringdata;
}

template<typename T>
const uint *QmlSharedPointer<T>::gen_data()
{
    return T::staticMetaObject.d.data;
}

template<typename T>
const QMetaObject * const *QmlSharedPointer<T>::gen_relatedMetaObjects()
{
    return T::staticMetaObject.d.relatedMetaObjects;
}

template<typename T>
void *QmlSharedPointer<T>::gen_extradata()
{
    return T::staticMetaObject.d.extradata;
}

QT_INIT_METAOBJECT template<typename T> const QMetaObject QmlSharedPointer<T>::staticMetaObject = { {
    QmlSharedPointer<T>::gen_superdata(),
    QmlSharedPointer<T>::gen_stringdata(),
    QmlSharedPointer<T>::gen_data(),
    QmlSharedPointer<T>::qt_static_metacall,
    QmlSharedPointer<T>::gen_relatedMetaObjects(),
    QmlSharedPointer<T>::gen_extradata(),
} };
