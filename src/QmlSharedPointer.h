#pragma once

#include <QSharedPointer>
#include <QMetaObject>
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
        for (int i = qObjectMethodCount; i < QmlSharedPointer<T>::data()->metaObject()->methodCount(); i++)
        {
            auto incomingSignal = QmlSharedPointer<T>::data()->metaObject()->method(i);
            if (incomingSignal.methodType() != QMetaMethod::Signal) continue;
            QMetaObject::connect(QmlSharedPointer<T>::data(), i, this, i);
        }
    }
    void deinit()
    {
        // Disconnect all signals from the encapsulated item to the QmlSharedPointer
        // Skip QObject's signals (e.g. destroyed)
        const int qObjectMethodCount = QObject::staticMetaObject.methodCount();
        for (int i = qObjectMethodCount; i < QmlSharedPointer<T>::data()->metaObject()->methodCount(); i++)
        {
            auto incomingSignal = QmlSharedPointer<T>::data()->metaObject()->method(i);
            if (incomingSignal.methodType() != QMetaMethod::Signal) continue;
            QMetaObject::disconnect(QmlSharedPointer<T>::data(), i, this, i);
        }
    }

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

    virtual const QMetaObject *metaObject() const
    {
        // This is sketchy but it works, and might be less sketchy than using QMetaObjectBuilder
        return QmlSharedPointer<T>::data()->metaObject();
    }

    virtual void *qt_metacast(const char *)
    {
        // Disable casting
        Q_ASSERT(false);
        return nullptr;
    }

    virtual int qt_metacall(QMetaObject::Call _c, int _id, void** _a)
    {
        //return QmlSharedPointer<T>::data()->qt_metacall(_c, _id, _a); // Dangerous??
        //Q_ASSERT(false);
        //return -1;
        if (_c == QMetaObject::InvokeMetaMethod) {
            auto method = metaObject()->method(_id);
            auto sig = QMetaObject::normalizedSignature(method.methodSignature());
            if (method.methodType() == QMetaMethod::Signal) {
                auto newId = _id - QObject::staticMetaObject.methodCount();
                QMetaObject::activate(this, metaObject(), newId, _a);
                return -1;
            } else {
                if (_id < QObject::staticMetaObject.methodCount()) {
                    // Handle things like deleteLater internally
                    return QObject::qt_metacall(_c, _id, _a);
                } else {
                    // Forward on anything else
                    return QmlSharedPointer<T>::data()->qt_metacall(_c, _id, _a); // This could be dangerous as _id might not be the same, but it appears to be the same so :shrug:
                }
            }
        } else if (_c == QMetaObject::IndexOfMethod ||
                   _c == QMetaObject::ReadProperty || 
                   _c == QMetaObject::WriteProperty ||
                   _c == QMetaObject::ResetProperty) {
            return QmlSharedPointer<T>::data()->qt_metacall(_c, _id, _a); // This could be dangerous as _id might not be the same
        } else {
            return -1;
        }
    }
};

struct qt_meta_stringdata_QmlSharedPointer_t {
    QByteArrayData data[1];
};

// This method gets run before main()
// and is very limited in what it can do,
// hence the old-school C string manipulation
template <typename T>
static const char *get_class_name_string() {
    static char c[128]{};
    strcpy(c, "QmlSharedPointer<");
    strncat(c, T::staticMetaObject.className(), 127);
    strncat(c, ">", 127);
    return c;
}

template <typename T> 
static const char *class_name_string = get_class_name_string<T>();

#define QT_MOC_LITERAL(idx, ptr) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET((int)strlen(ptr), \
    qptrdiff(qintptr(ptr) - qintptr(&qt_meta_stringdata_QmlSharedPointer<T>) \
        - idx * sizeof(QByteArrayData)) \
    )
template <typename T> static const qt_meta_stringdata_QmlSharedPointer_t qt_meta_stringdata_QmlSharedPointer = {
    {
        QT_MOC_LITERAL(0, class_name_string<T>), // "QmlSharedPointer<ExampleQmlObject>"
    }
    //"QmlSharedPointer<" + T::staticMetaObject.className() + ">\0"
};

#undef QT_MOC_LITERAL

static const uint qt_meta_data_QmlSharedPointer[] = {
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       //6,   14, // methods
       //2,   62, // properties
       //0,    0, // enums/sets
       //0,    0, // constructors
       //0,       // flags
       //3,       // signalCount
};

template <typename T> void QmlSharedPointer<T>::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    return;
}

//QT_INIT_METAOBJECT template<typename T> const QMetaObject QmlSharedPointer<T>::staticMetaObject{};
QT_INIT_METAOBJECT template<typename T> const QMetaObject QmlSharedPointer<T>::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_QmlSharedPointer<T>.data,
    qt_meta_data_QmlSharedPointer,
    qt_static_metacall,
    nullptr,
    nullptr
} };
