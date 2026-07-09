/****************************************************************************
** Meta object code from reading C++ file 'engine.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/engine.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'engine.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN6EngineE_t {};
} // unnamed namespace

template <> constexpr inline auto Engine::qt_create_metaobjectdata<qt_meta_tag_ZN6EngineE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Engine",
        "QML.Element",
        "auto",
        "QML.Singleton",
        "true",
        "recalculated",
        "",
        "set",
        "key",
        "value",
        "restaurarValoresIniciales",
        "exportarPdf",
        "inputs",
        "QVariantMap",
        "datosBase",
        "financiacion",
        "personal",
        "proyeccion",
        "QVariantList",
        "impuestos",
        "analisis",
        "banco",
        "AmortModel*",
        "cooperativa",
        "propiedades",
        "rutaDatos"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'recalculated'
        QtMocHelpers::SignalData<void()>(5, 6, QMC::AccessPublic, QMetaType::Void),
        // Method 'set'
        QtMocHelpers::MethodData<void(const QString &, double)>(7, 6, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { QMetaType::Double, 9 },
        }}),
        // Method 'restaurarValoresIniciales'
        QtMocHelpers::MethodData<void()>(10, 6, QMC::AccessPublic, QMetaType::Void),
        // Method 'exportarPdf'
        QtMocHelpers::MethodData<QString()>(11, 6, QMC::AccessPublic, QMetaType::QString),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'inputs'
        QtMocHelpers::PropertyData<QVariantMap>(12, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'datosBase'
        QtMocHelpers::PropertyData<QVariantMap>(14, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'financiacion'
        QtMocHelpers::PropertyData<QVariantMap>(15, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'personal'
        QtMocHelpers::PropertyData<QVariantMap>(16, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'proyeccion'
        QtMocHelpers::PropertyData<QVariantList>(17, 0x80000000 | 18, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'impuestos'
        QtMocHelpers::PropertyData<QVariantMap>(19, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'analisis'
        QtMocHelpers::PropertyData<QVariantMap>(20, 0x80000000 | 13, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'banco'
        QtMocHelpers::PropertyData<AmortModel*>(21, 0x80000000 | 22, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'cooperativa'
        QtMocHelpers::PropertyData<AmortModel*>(23, 0x80000000 | 22, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'propiedades'
        QtMocHelpers::PropertyData<AmortModel*>(24, 0x80000000 | 22, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'rutaDatos'
        QtMocHelpers::PropertyData<QString>(25, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Constant),
    };
    QtMocHelpers::UintData qt_enums {
    };
    QtMocHelpers::UintData qt_constructors {};
    QtMocHelpers::ClassInfos qt_classinfo({
            {    1,    2 },
            {    3,    4 },
    });
    return QtMocHelpers::metaObjectData<Engine, void>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums, qt_constructors, qt_classinfo);
}
Q_CONSTINIT const QMetaObject Engine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6EngineE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6EngineE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6EngineE_t>.metaTypes,
    nullptr
} };

void Engine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Engine *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->recalculated(); break;
        case 1: _t->set((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2]))); break;
        case 2: _t->restaurarValoresIniciales(); break;
        case 3: { QString _r = _t->exportarPdf();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Engine::*)()>(_a, &Engine::recalculated, 0))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QVariantMap*>(_v) = _t->inputs(); break;
        case 1: *reinterpret_cast<QVariantMap*>(_v) = _t->datosBase(); break;
        case 2: *reinterpret_cast<QVariantMap*>(_v) = _t->financiacion(); break;
        case 3: *reinterpret_cast<QVariantMap*>(_v) = _t->personal(); break;
        case 4: *reinterpret_cast<QVariantList*>(_v) = _t->proyeccion(); break;
        case 5: *reinterpret_cast<QVariantMap*>(_v) = _t->impuestos(); break;
        case 6: *reinterpret_cast<QVariantMap*>(_v) = _t->analisis(); break;
        case 7: *reinterpret_cast<AmortModel**>(_v) = _t->banco(); break;
        case 8: *reinterpret_cast<AmortModel**>(_v) = _t->cooperativa(); break;
        case 9: *reinterpret_cast<AmortModel**>(_v) = _t->propiedades(); break;
        case 10: *reinterpret_cast<QString*>(_v) = _t->rutaDatos(); break;
        default: break;
        }
    }
}

const QMetaObject *Engine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Engine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6EngineE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Engine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void Engine::recalculated()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
