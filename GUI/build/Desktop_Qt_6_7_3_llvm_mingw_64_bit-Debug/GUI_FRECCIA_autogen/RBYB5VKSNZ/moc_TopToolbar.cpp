/****************************************************************************
** Meta object code from reading C++ file 'TopToolbar.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../Header_Files/TopToolbar.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TopToolbar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.7.3. It"
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

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSTopToolbarENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSTopToolbarENDCLASS = QtMocHelpers::stringData(
    "TopToolbar",
    "updateRecordingStatus",
    "",
    "recording",
    "updateTimerLabel",
    "tiempo",
    "updateReceptionStatus",
    "enabled",
    "updateModoArchivo",
    "fileName",
    "updateSerialConfig",
    "port",
    "baud",
    "ok",
    "onBtnRecordClicked",
    "onBtnStopClicked",
    "onBtnVerAntiguosClicked",
    "onBtnResetClicked",
    "onAction2DTriggered",
    "onAction3DTriggered",
    "onActionConfigComTriggered",
    "onActionCerrarTriggered"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSTopToolbarENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   92,    2, 0x08,    1 /* Private */,
       4,    1,   95,    2, 0x08,    3 /* Private */,
       6,    1,   98,    2, 0x08,    5 /* Private */,
       8,    2,  101,    2, 0x08,    7 /* Private */,
      10,    3,  106,    2, 0x08,   10 /* Private */,
      14,    0,  113,    2, 0x08,   14 /* Private */,
      15,    0,  114,    2, 0x08,   15 /* Private */,
      16,    0,  115,    2, 0x08,   16 /* Private */,
      17,    0,  116,    2, 0x08,   17 /* Private */,
      18,    0,  117,    2, 0x08,   18 /* Private */,
      19,    0,  118,    2, 0x08,   19 /* Private */,
      20,    0,  119,    2, 0x08,   20 /* Private */,
      21,    0,  120,    2, 0x08,   21 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    9,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Bool,   11,   12,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject TopToolbar::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSTopToolbarENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSTopToolbarENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSTopToolbarENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TopToolbar, std::true_type>,
        // method 'updateRecordingStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'updateTimerLabel'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateReceptionStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'updateModoArchivo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateSerialConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onBtnRecordClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBtnStopClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBtnVerAntiguosClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBtnResetClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAction2DTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAction3DTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onActionConfigComTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onActionCerrarTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void TopToolbar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TopToolbar *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateRecordingStatus((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->updateTimerLabel((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->updateReceptionStatus((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->updateModoArchivo((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->updateSerialConfig((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 5: _t->onBtnRecordClicked(); break;
        case 6: _t->onBtnStopClicked(); break;
        case 7: _t->onBtnVerAntiguosClicked(); break;
        case 8: _t->onBtnResetClicked(); break;
        case 9: _t->onAction2DTriggered(); break;
        case 10: _t->onAction3DTriggered(); break;
        case 11: _t->onActionConfigComTriggered(); break;
        case 12: _t->onActionCerrarTriggered(); break;
        default: ;
        }
    }
}

const QMetaObject *TopToolbar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TopToolbar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSTopToolbarENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TopToolbar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 13;
    }
    return _id;
}
QT_WARNING_POP
