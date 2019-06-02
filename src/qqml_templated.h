#pragma once


/* This class exists due to a bug in Qt
   where qmlRegisterType does not handle templates correctly.
   When it generates the QQmlListProperty<> that wraps the given type,
   the typename is not normalized according to Qt's rules.

   You can use these functions to register non-templated types
   as well.
*/

#include <QtQml/qqmlprivate.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmlpropertyvaluesource.h>
#include <QtQml/qqmllist.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qmetaobject.h>

#define QML_GETTYPENAMES_TEMPLATED \
    const char *className = T::staticMetaObject.className(); \
    const int nameLen = int(strlen(className)); \
    QVarLengthArray<char,48> pointerName(nameLen+2); \
    memcpy(pointerName.data(), className, size_t(nameLen)); \
    pointerName[nameLen] = '*'; \
    pointerName[nameLen+1] = '\0'; \
    const int listLen = int(strlen("QQmlListProperty<")); \
    const bool templated = (className[nameLen - 1] == '>'); \
    QVarLengthArray<char,64> listName(listLen + nameLen + 2 + (templated ? 1 : 0)); \
    memcpy(listName.data(), "QQmlListProperty<", size_t(listLen)); \
    memcpy(listName.data()+listLen, className, size_t(nameLen)); \
    if (templated) { \
        listName[listLen+nameLen] = ' '; \
        listName[listLen+nameLen+1] = '>'; \
        listName[listLen+nameLen+2] = '\0'; \
    } else { \
        listName[listLen+nameLen] = '>'; \
        listName[listLen+nameLen+1] = '\0'; \
    }


template<typename T>
int qmlRegisterTemplatedType(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QML_GETTYPENAMES_TEMPLATED

    QQmlPrivate::RegisterType type = {
        0,

        qRegisterNormalizedMetaType<T *>(pointerName.constData()),
        qRegisterNormalizedMetaType<QQmlListProperty<T> >(listName.constData()),
        sizeof(T), QQmlPrivate::createInto<T>,
        QString(),

        uri, versionMajor, versionMinor, qmlName, &T::staticMetaObject,

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        0
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

template<typename T>
int qmlRegisterUncreatableTemplatedType(const char *uri, int versionMajor, int versionMinor, const char *qmlName, const QString& reason)
{
    QML_GETTYPENAMES_TEMPLATED

    QQmlPrivate::RegisterType type = {
        0,

        qRegisterNormalizedMetaType<T *>(pointerName.constData()),
        qRegisterNormalizedMetaType<QQmlListProperty<T> >(listName.constData()),
        0,
        nullptr,
        reason,

        uri, versionMajor, versionMinor, qmlName, &T::staticMetaObject,

        QQmlPrivate::attachedPropertiesFunc<T>(),
        QQmlPrivate::attachedPropertiesMetaObject<T>(),

        QQmlPrivate::StaticCastSelector<T,QQmlParserStatus>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueSource>::cast(),
        QQmlPrivate::StaticCastSelector<T,QQmlPropertyValueInterceptor>::cast(),

        nullptr, nullptr,

        nullptr,
        0
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}
