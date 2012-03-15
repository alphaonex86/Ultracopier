/****************************************************************************
** Meta object code from reading C++ file 'writeThread.h'
**
** Created: Wed Mar 7 11:14:56 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Ultracopier-0.2/writeThread.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'writeThread.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_writeThread[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   12,   12,   12, 0x05,
      38,   12,   12,   12, 0x05,
      66,   64,   12,   12, 0x05,
     128,   97,   12,   12, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_writeThread[] = {
    "writeThread\0\0haveStartFileOperation()\0"
    "haveFinishFileOperation()\0,\0"
    "errorOnFile(QFileInfo,QString)\0"
    "level,fonction,text,file,ligne\0"
    "debugInformation(DebugLevel,QString,QString,QString,int)\0"
};

void writeThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        writeThread *_t = static_cast<writeThread *>(_o);
        switch (_id) {
        case 0: _t->haveStartFileOperation(); break;
        case 1: _t->haveFinishFileOperation(); break;
        case 2: _t->errorOnFile((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 3: _t->debugInformation((*reinterpret_cast< DebugLevel(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData writeThread::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject writeThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_writeThread,
      qt_meta_data_writeThread, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &writeThread::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *writeThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *writeThread::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_writeThread))
        return static_cast<void*>(const_cast< writeThread*>(this));
    return QThread::qt_metacast(_clname);
}

int writeThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void writeThread::haveStartFileOperation()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void writeThread::haveFinishFileOperation()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void writeThread::errorOnFile(QFileInfo _t1, QString _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void writeThread::debugInformation(DebugLevel _t1, QString _t2, QString _t3, QString _t4, int _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE
