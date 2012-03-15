/****************************************************************************
** Meta object code from reading C++ file 'scanFileOrFolder.h'
**
** Created: Wed Mar 7 11:14:56 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Ultracopier-0.2/scanFileOrFolder.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scanFileOrFolder.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_scanFileOrFolder[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      50,   18,   17,   17, 0x05,
     105,   86,   17,   17, 0x05,
     170,  139,   17,   17, 0x05,
     253,  227,   17,   17, 0x05,
     320,  299,   17,   17, 0x05,

 // slots: signature, parameters, type, tag, flags
     373,  353,   17,   17, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_scanFileOrFolder[] = {
    "scanFileOrFolder\0\0source,destination,numberOfItem\0"
    "folderTransfer(QString,QString,int)\0"
    "source,destination\0fileTransfer(QFileInfo,QFileInfo)\0"
    "level,fonction,text,file,ligne\0"
    "debugInformation(DebugLevel,QString,QString,QString,int)\0"
    "source,isSame,destination\0"
    "folderAlreadyExists(QFileInfo,bool,QFileInfo)\0"
    "fileInfo,errorString\0"
    "errorOnFolder(QFileInfo,QString)\0"
    "sources,destination\0addToList(QStringList,QString)\0"
};

void scanFileOrFolder::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        scanFileOrFolder *_t = static_cast<scanFileOrFolder *>(_o);
        switch (_id) {
        case 0: _t->folderTransfer((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 1: _t->fileTransfer((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QFileInfo(*)>(_a[2]))); break;
        case 2: _t->debugInformation((*reinterpret_cast< DebugLevel(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5]))); break;
        case 3: _t->folderAlreadyExists((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< QFileInfo(*)>(_a[3]))); break;
        case 4: _t->errorOnFolder((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 5: _t->addToList((*reinterpret_cast< const QStringList(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData scanFileOrFolder::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject scanFileOrFolder::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_scanFileOrFolder,
      qt_meta_data_scanFileOrFolder, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &scanFileOrFolder::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *scanFileOrFolder::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *scanFileOrFolder::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_scanFileOrFolder))
        return static_cast<void*>(const_cast< scanFileOrFolder*>(this));
    return QThread::qt_metacast(_clname);
}

int scanFileOrFolder::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void scanFileOrFolder::folderTransfer(QString _t1, QString _t2, int _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void scanFileOrFolder::fileTransfer(QFileInfo _t1, QFileInfo _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void scanFileOrFolder::debugInformation(DebugLevel _t1, QString _t2, QString _t3, QString _t4, int _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void scanFileOrFolder::folderAlreadyExists(QFileInfo _t1, bool _t2, QFileInfo _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void scanFileOrFolder::errorOnFolder(QFileInfo _t1, QString _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_END_MOC_NAMESPACE
