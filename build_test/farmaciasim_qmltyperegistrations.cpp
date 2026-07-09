/****************************************************************************
** Generated QML type registration code
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <QtQml/qqml.h>
#include <QtQml/qqmlmoduleregistration.h>

#if __has_include(<amortmodel.h>)
#  include <amortmodel.h>
#endif
#if __has_include(<engine.h>)
#  include <engine.h>
#endif


#if !defined(QT_STATIC)
#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT
#else
#define Q_QMLTYPE_EXPORT
#endif
Q_QMLTYPE_EXPORT void qml_register_types_FarmaciaSim()
{
    QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    qmlRegisterTypesAndRevisions<AmortModel>("FarmaciaSim", 1);
    qmlRegisterTypesAndRevisions<Engine>("FarmaciaSim", 1);
    QMetaType::fromType<QAbstractItemModel *>().id();
    qmlRegisterEnum<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint");
    qmlRegisterEnum<QAbstractItemModel::CheckIndexOption>("QAbstractItemModel::CheckIndexOption");
    QMetaType::fromType<QAbstractTableModel *>().id();
    QT_WARNING_POP
    qmlRegisterModule("FarmaciaSim", 1, 0);
}

static const QQmlModuleRegistration farmaciaSimRegistration("FarmaciaSim", qml_register_types_FarmaciaSim);
