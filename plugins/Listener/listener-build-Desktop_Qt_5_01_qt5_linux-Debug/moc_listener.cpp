/****************************************************************************
** Meta object code from reading C++ file 'listener.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.0.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../catchcopy-v0002/listener.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'listener.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.0.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_Listener_t {
    QByteArrayData data[17];
    char stringdata[186];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    offsetof(qt_meta_stringdata_Listener_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    )
static const qt_meta_stringdata_Listener_t qt_meta_stringdata_Listener = {
    {
QT_MOC_LITERAL(0, 0, 8),
QT_MOC_LITERAL(1, 9, 16),
QT_MOC_LITERAL(2, 26, 0),
QT_MOC_LITERAL(3, 27, 7),
QT_MOC_LITERAL(4, 35, 9),
QT_MOC_LITERAL(5, 45, 16),
QT_MOC_LITERAL(6, 62, 17),
QT_MOC_LITERAL(7, 80, 5),
QT_MOC_LITERAL(8, 86, 10),
QT_MOC_LITERAL(9, 97, 6),
QT_MOC_LITERAL(10, 104, 4),
QT_MOC_LITERAL(11, 109, 22),
QT_MOC_LITERAL(12, 132, 7),
QT_MOC_LITERAL(13, 140, 4),
QT_MOC_LITERAL(14, 145, 11),
QT_MOC_LITERAL(15, 157, 22),
QT_MOC_LITERAL(16, 180, 4)
    },
    "Listener\0transferFinished\0\0orderId\0"
    "withError\0transferCanceled\0newLanguageLoaded\0"
    "error\0clientName\0client\0name\0"
    "copyWithoutDestination\0sources\0copy\0"
    "destination\0moveWithoutDestination\0"
    "move\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Listener[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   59,    2, 0x0a,
       5,    1,   64,    2, 0x0a,
       6,    0,   67,    2, 0x0a,
       7,    1,   68,    2, 0x08,
       8,    2,   71,    2, 0x08,
      11,    2,   76,    2, 0x08,
      13,    3,   81,    2, 0x08,
      15,    2,   88,    2, 0x08,
      16,    3,   93,    2, 0x08,

 // slots: parameters
    QMetaType::Void, QMetaType::UInt, QMetaType::Bool,    3,    4,
    QMetaType::Void, QMetaType::UInt,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::UInt, QMetaType::QString,    9,   10,
    QMetaType::Void, QMetaType::UInt, QMetaType::QStringList,    3,   12,
    QMetaType::Void, QMetaType::UInt, QMetaType::QStringList, QMetaType::QString,    3,   12,   14,
    QMetaType::Void, QMetaType::UInt, QMetaType::QStringList,    3,   12,
    QMetaType::Void, QMetaType::UInt, QMetaType::QStringList, QMetaType::QString,    3,   12,   14,

       0        // eod
};

void Listener::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Listener *_t = static_cast<Listener *>(_o);
        switch (_id) {
        case 0: _t->transferFinished((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2]))); break;
        case 1: _t->transferCanceled((*reinterpret_cast< const quint32(*)>(_a[1]))); break;
        case 2: _t->newLanguageLoaded(); break;
        case 3: _t->error((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 4: _t->clientName((*reinterpret_cast< quint32(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 5: _t->copyWithoutDestination((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2]))); break;
        case 6: _t->copy((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 7: _t->moveWithoutDestination((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2]))); break;
        case 8: _t->move((*reinterpret_cast< const quint32(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        default: ;
        }
    }
}

const QMetaObject Listener::staticMetaObject = {
    { &PluginInterface_Listener::staticMetaObject, qt_meta_stringdata_Listener.data,
      qt_meta_data_Listener,  qt_static_metacall, 0, 0}
};


const QMetaObject *Listener::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Listener::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Listener.stringdata))
        return static_cast<void*>(const_cast< Listener*>(this));
    if (!strcmp(_clname, "first-world.info.ultracopier.PluginInterface.Listener/1.0.0.0"))
        return static_cast< PluginInterface_Listener*>(const_cast< Listener*>(this));
    return PluginInterface_Listener::qt_metacast(_clname);
}

int Listener::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PluginInterface_Listener::qt_metacall(_c, _id, _a);
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

QT_PLUGIN_METADATA_SECTION const uint qt_section_alignment_dummy = 42;

#ifdef QT_NO_DEBUG

QT_PLUGIN_METADATA_SECTION
static const unsigned char qt_pluginMetaData[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', ' ',
    0x71, 0x62, 0x6a, 0x73, 0x01, 0x00, 0x00, 0x00,
    0xc0, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
    0xac, 0x00, 0x00, 0x00, 0x1b, 0x03, 0x00, 0x00,
    0x03, 0x00, 0x49, 0x49, 0x44, 0x00, 0x00, 0x00,
    0x3d, 0x00, 0x66, 0x69, 0x72, 0x73, 0x74, 0x2d,
    0x77, 0x6f, 0x72, 0x6c, 0x64, 0x2e, 0x69, 0x6e,
    0x66, 0x6f, 0x2e, 0x75, 0x6c, 0x74, 0x72, 0x61,
    0x63, 0x6f, 0x70, 0x69, 0x65, 0x72, 0x2e, 0x50,
    0x6c, 0x75, 0x67, 0x69, 0x6e, 0x49, 0x6e, 0x74,
    0x65, 0x72, 0x66, 0x61, 0x63, 0x65, 0x2e, 0x4c,
    0x69, 0x73, 0x74, 0x65, 0x6e, 0x65, 0x72, 0x2f,
    0x31, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x30, 0x00,
    0x1b, 0x0d, 0x00, 0x00, 0x09, 0x00, 0x63, 0x6c,
    0x61, 0x73, 0x73, 0x4e, 0x61, 0x6d, 0x65, 0x00,
    0x08, 0x00, 0x4c, 0x69, 0x73, 0x74, 0x65, 0x6e,
    0x65, 0x72, 0x00, 0x00, 0x3a, 0x00, 0xa0, 0x00,
    0x07, 0x00, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f,
    0x6e, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x64, 0x65, 0x62, 0x75, 0x67, 0x00,
    0x15, 0x14, 0x00, 0x00, 0x08, 0x00, 0x4d, 0x65,
    0x74, 0x61, 0x44, 0x61, 0x74, 0x61, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x90, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00,
    0x84, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00
};

#else // QT_NO_DEBUG

QT_PLUGIN_METADATA_SECTION
static const unsigned char qt_pluginMetaData[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', ' ',
    0x71, 0x62, 0x6a, 0x73, 0x01, 0x00, 0x00, 0x00,
    0xc0, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
    0xac, 0x00, 0x00, 0x00, 0x1b, 0x03, 0x00, 0x00,
    0x03, 0x00, 0x49, 0x49, 0x44, 0x00, 0x00, 0x00,
    0x3d, 0x00, 0x66, 0x69, 0x72, 0x73, 0x74, 0x2d,
    0x77, 0x6f, 0x72, 0x6c, 0x64, 0x2e, 0x69, 0x6e,
    0x66, 0x6f, 0x2e, 0x75, 0x6c, 0x74, 0x72, 0x61,
    0x63, 0x6f, 0x70, 0x69, 0x65, 0x72, 0x2e, 0x50,
    0x6c, 0x75, 0x67, 0x69, 0x6e, 0x49, 0x6e, 0x74,
    0x65, 0x72, 0x66, 0x61, 0x63, 0x65, 0x2e, 0x4c,
    0x69, 0x73, 0x74, 0x65, 0x6e, 0x65, 0x72, 0x2f,
    0x31, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x30, 0x00,
    0x15, 0x0d, 0x00, 0x00, 0x08, 0x00, 0x4d, 0x65,
    0x74, 0x61, 0x44, 0x61, 0x74, 0x61, 0x00, 0x00,
    0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x9b, 0x10, 0x00, 0x00,
    0x09, 0x00, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x4e,
    0x61, 0x6d, 0x65, 0x00, 0x08, 0x00, 0x4c, 0x69,
    0x73, 0x74, 0x65, 0x6e, 0x65, 0x72, 0x00, 0x00,
    0x31, 0x00, 0x00, 0x00, 0x05, 0x00, 0x64, 0x65,
    0x62, 0x75, 0x67, 0x00, 0x3a, 0x00, 0xa0, 0x00,
    0x07, 0x00, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f,
    0x6e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x58, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00,
    0x90, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00
};
#endif // QT_NO_DEBUG

QT_MOC_EXPORT_PLUGIN(Listener, Listener)

QT_END_MOC_NAMESPACE
