/****************************************************************************
** Meta object code from reading C++ file 'PluginInterface_Listener.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.0.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../interface/PluginInterface_Listener.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PluginInterface_Listener.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.0.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_PluginInterface_Listener_t {
    QByteArrayData data[23];
    char stringdata[300];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_PluginInterface_Listener_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_PluginInterface_Listener_t qt_meta_stringdata_PluginInterface_Listener = {
    {
QT_MOC_LITERAL(0, 0, 24),
QT_MOC_LITERAL(1, 25, 8),
QT_MOC_LITERAL(2, 34, 0),
QT_MOC_LITERAL(3, 35, 27),
QT_MOC_LITERAL(4, 63, 5),
QT_MOC_LITERAL(5, 69, 25),
QT_MOC_LITERAL(6, 95, 7),
QT_MOC_LITERAL(7, 103, 7),
QT_MOC_LITERAL(8, 111, 7),
QT_MOC_LITERAL(9, 119, 11),
QT_MOC_LITERAL(10, 131, 25),
QT_MOC_LITERAL(11, 157, 7),
QT_MOC_LITERAL(12, 165, 16),
QT_MOC_LITERAL(13, 182, 23),
QT_MOC_LITERAL(14, 206, 5),
QT_MOC_LITERAL(15, 212, 8),
QT_MOC_LITERAL(16, 221, 4),
QT_MOC_LITERAL(17, 226, 4),
QT_MOC_LITERAL(18, 231, 5),
QT_MOC_LITERAL(19, 237, 16),
QT_MOC_LITERAL(20, 254, 9),
QT_MOC_LITERAL(21, 264, 16),
QT_MOC_LITERAL(22, 281, 17)
    },
    "PluginInterface_Listener\0newState\0\0"
    "Ultracopier::ListeningState\0state\0"
    "newCopyWithoutDestination\0orderId\0"
    "sources\0newCopy\0destination\0"
    "newMoveWithoutDestination\0newMove\0"
    "debugInformation\0Ultracopier::DebugLevel\0"
    "level\0fonction\0text\0file\0ligne\0"
    "transferFinished\0withError\0transferCanceled\0"
    "newLanguageLoaded\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PluginInterface_Listener[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x05,
       5,    2,   62,    2, 0x05,
       8,    3,   67,    2, 0x05,
      10,    2,   74,    2, 0x05,
      11,    3,   79,    2, 0x05,
      12,    5,   86,    2, 0x05,

 // slots: name, argc, parameters, tag, flags
      19,    2,   97,    2, 0x0a,
      21,    1,  102,    2, 0x0a,
      22,    0,  105,    2, 0x0a,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::UInt, QMetaType::QStringList,    6,    7,
    QMetaType::Void, QMetaType::UInt, QMetaType::QStringList, QMetaType::QString,    6,    7,    9,
    QMetaType::Void, QMetaType::UInt, QMetaType::QStringList,    6,    7,
    QMetaType::Void, QMetaType::UInt, QMetaType::QStringList, QMetaType::QString,    6,    7,    9,
    QMetaType::Void, 0x80000000 | 13, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::Int,   14,   15,   16,   17,   18,

 // slots: parameters
    QMetaType::Void, QMetaType::UInt, QMetaType::Bool,    6,   20,
    QMetaType::Void, QMetaType::UInt,    6,
    QMetaType::Void,

       0        // eod
};

void PluginInterface_Listener::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        PluginInterface_Listener *_t = static_cast<PluginInterface_Listener *>(_o);
        switch (_id) {
        case 0: _t->newState((*reinterpret_cast< const Ultracopier::ListeningState(*)>(_a[1]))); break;
        case 1: _t->newCopyWithoutDestination((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2]))); break;
        case 2: _t->newCopy((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 3: _t->newMoveWithoutDestination((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2]))); break;
        case 4: _t->newMove((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 5: _t->debugInformation((*reinterpret_cast< const Ultracopier::DebugLevel(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4])),(*reinterpret_cast< const int(*)>(_a[5]))); break;
        case 6: _t->transferFinished((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2]))); break;
        case 7: _t->transferCanceled((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 8: _t->newLanguageLoaded(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (PluginInterface_Listener::*_t)(const Ultracopier::ListeningState & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&PluginInterface_Listener::newState)) {
                *result = 0;
            }
        }
        {
            typedef void (PluginInterface_Listener::*_t)(const quint32 & , const QStringList & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&PluginInterface_Listener::newCopyWithoutDestination)) {
                *result = 1;
            }
        }
        {
            typedef void (PluginInterface_Listener::*_t)(const quint32 & , const QStringList & , const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&PluginInterface_Listener::newCopy)) {
                *result = 2;
            }
        }
        {
            typedef void (PluginInterface_Listener::*_t)(const quint32 & , const QStringList & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&PluginInterface_Listener::newMoveWithoutDestination)) {
                *result = 3;
            }
        }
        {
            typedef void (PluginInterface_Listener::*_t)(const quint32 & , const QStringList & , const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&PluginInterface_Listener::newMove)) {
                *result = 4;
            }
        }
        {
            typedef void (PluginInterface_Listener::*_t)(const Ultracopier::DebugLevel & , const QString & , const QString & , const QString & , const int & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&PluginInterface_Listener::debugInformation)) {
                *result = 5;
            }
        }
    }
}

const QMetaObject PluginInterface_Listener::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_PluginInterface_Listener.data,
      qt_meta_data_PluginInterface_Listener,  qt_static_metacall, 0, 0}
};


const QMetaObject *PluginInterface_Listener::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PluginInterface_Listener::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PluginInterface_Listener.stringdata))
        return static_cast<void*>(const_cast< PluginInterface_Listener*>(this));
    return QObject::qt_metacast(_clname);
}

int PluginInterface_Listener::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void PluginInterface_Listener::newState(const Ultracopier::ListeningState & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PluginInterface_Listener::newCopyWithoutDestination(const quint32 & _t1, const QStringList & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void PluginInterface_Listener::newCopy(const quint32 & _t1, const QStringList & _t2, const QString & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void PluginInterface_Listener::newMoveWithoutDestination(const quint32 & _t1, const QStringList & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void PluginInterface_Listener::newMove(const quint32 & _t1, const QStringList & _t2, const QString & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void PluginInterface_Listener::debugInformation(const Ultracopier::DebugLevel & _t1, const QString & _t2, const QString & _t3, const QString & _t4, const int & _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_END_MOC_NAMESPACE
