/****************************************************************************
** Meta object code from reading C++ file 'SensorManager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../Header_Files/SensorManager.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SensorManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.5.3. It"
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
struct qt_meta_stringdata_CLASSSensorManagerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSSensorManagerENDCLASS = QtMocHelpers::stringData(
    "SensorManager",
    "newSensorData",
    "",
    "SensorData",
    "data",
    "serialReconfigured",
    "port",
    "baud",
    "ok",
    "setSerial",
    "portName",
    "clearData",
    "setReceivingEnabled",
    "enabled"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSSensorManagerENDCLASS_t {
    uint offsetsAndSizes[28];
    char stringdata0[14];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[5];
    char stringdata5[19];
    char stringdata6[5];
    char stringdata7[5];
    char stringdata8[3];
    char stringdata9[10];
    char stringdata10[9];
    char stringdata11[10];
    char stringdata12[20];
    char stringdata13[8];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSSensorManagerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSSensorManagerENDCLASS_t qt_meta_stringdata_CLASSSensorManagerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 13),  // "SensorManager"
        QT_MOC_LITERAL(14, 13),  // "newSensorData"
        QT_MOC_LITERAL(28, 0),  // ""
        QT_MOC_LITERAL(29, 10),  // "SensorData"
        QT_MOC_LITERAL(40, 4),  // "data"
        QT_MOC_LITERAL(45, 18),  // "serialReconfigured"
        QT_MOC_LITERAL(64, 4),  // "port"
        QT_MOC_LITERAL(69, 4),  // "baud"
        QT_MOC_LITERAL(74, 2),  // "ok"
        QT_MOC_LITERAL(77, 9),  // "setSerial"
        QT_MOC_LITERAL(87, 8),  // "portName"
        QT_MOC_LITERAL(96, 9),  // "clearData"
        QT_MOC_LITERAL(106, 19),  // "setReceivingEnabled"
        QT_MOC_LITERAL(126, 7)   // "enabled"
    },
    "SensorManager",
    "newSensorData",
    "",
    "SensorData",
    "data",
    "serialReconfigured",
    "port",
    "baud",
    "ok",
    "setSerial",
    "portName",
    "clearData",
    "setReceivingEnabled",
    "enabled"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSSensorManagerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       5,    3,   53,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       9,    2,   60,    2, 0x0a,    7 /* Public */,
       9,    1,   65,    2, 0x2a,   10 /* Public | MethodCloned */,
      11,    0,   68,    2, 0x0a,   12 /* Public */,
      12,    1,   69,    2, 0x0a,   13 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Bool,    6,    7,    8,

 // slots: parameters
    QMetaType::Bool, QMetaType::QString, QMetaType::Int,   10,    7,
    QMetaType::Bool, QMetaType::QString,   10,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   13,

       0        // eod
};

Q_CONSTINIT const QMetaObject SensorManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSSensorManagerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSSensorManagerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSSensorManagerENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SensorManager, std::true_type>,
        // method 'newSensorData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const SensorData &, std::false_type>,
        // method 'serialReconfigured'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setSerial'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setSerial'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clearData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setReceivingEnabled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void SensorManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SensorManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->newSensorData((*reinterpret_cast< std::add_pointer_t<SensorData>>(_a[1]))); break;
        case 1: _t->serialReconfigured((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 2: { bool _r = _t->setSerial((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: { bool _r = _t->setSerial((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 4: _t->clearData(); break;
        case 5: _t->setReceivingEnabled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SensorManager::*)(const SensorData & );
            if (_t _q_method = &SensorManager::newSensorData; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SensorManager::*)(QString , int , bool );
            if (_t _q_method = &SensorManager::serialReconfigured; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *SensorManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SensorManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSSensorManagerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SensorManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void SensorManager::newSensorData(const SensorData & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SensorManager::serialReconfigured(QString _t1, int _t2, bool _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
