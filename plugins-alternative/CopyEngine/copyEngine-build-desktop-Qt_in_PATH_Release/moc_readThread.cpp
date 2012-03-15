/****************************************************************************
** Meta object code from reading C++ file 'readThread.h'
**
** Created: Wed Mar 7 11:14:57 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Ultracopier-0.2/readThread.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'readThread.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_readThread[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      38,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x05,
      34,   11,   11,   11, 0x05,
      64,   61,   11,   11, 0x05,
     108,   61,   11,   11, 0x05,
     165,  144,   11,   11, 0x05,
     213,   11,   11,   11, 0x05,
     237,   11,   11,   11, 0x05,
     261,   11,   11,   11, 0x05,
     297,  294,   11,   11, 0x05,
     322,   11,   11,   11, 0x05,
     371,  340,   11,   11, 0x05,
     428,   11,   11,   11, 0x05,
     445,  440,   11,   11, 0x05,
     461,  440,   11,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
     493,  477,   11,   11, 0x0a,
     525,  516,   11,   11, 0x0a,
     556,  543,   11,   11, 0x0a,
     582,  573,   11,   11, 0x0a,
     626,  616,  611,   11, 0x0a,
     644,   11,   11,   11, 0x0a,
     660,   11,   11,   11, 0x0a,
     677,   11,   11,   11, 0x0a,
     695,  294,   11,   11, 0x0a,
     728,  724,   11,   11, 0x0a,
     752,  724,   11,   11, 0x0a,
     779,  724,   11,   11, 0x0a,
     803,  724,   11,   11, 0x0a,
     829,  724,   11,   11, 0x0a,
     859,   11,   11,   11, 0x08,
     882,   11,   11,   11, 0x08,
     905,   11,   11,   11, 0x08,
     934,   11,   11,   11, 0x08,
     962,  294,   11,   11, 0x08,
     986,  984,   11,   11, 0x08,
    1031,   11,   11,   11, 0x08,
    1047,   11,   11,   11, 0x08,
    1064,   11,  611,   11, 0x08,
    1086,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_readThread[] = {
    "readThread\0\0queryNewWriteThread()\0"
    "queryStartWriteThread(int)\0,,\0"
    "fileAlreadyExists(QFileInfo,QFileInfo,bool)\0"
    "errorOnFile(QFileInfo,QString,bool)\0"
    ",,indexOfWriteThread\0"
    "errorOnFileInWriteThread(QFileInfo,QString,int)\0"
    "transferIsInPause(bool)\0transferIsStarted(bool)\0"
    "newTransferStart(ItemOfCopyList)\0id\0"
    "newTransferStop(quint64)\0newActionOnList()\0"
    "level,fonction,text,file,ligne\0"
    "debugInformation(DebugLevel,QString,QString,QString,int)\0"
    "cancelAll()\0path\0rmPath(QString)\0"
    "mkPath(QString)\0doRightTransfer\0"
    "setRightTransfer(bool)\0keepDate\0"
    "setKeepDate(bool)\0tempMaxSpeed\0"
    "setMaxSpeed(int)\0prealloc\0"
    "setPreallocateFileSize(bool)\0bool\0"
    "blockSize\0setBlockSize(int)\0pauseTransfer()\0"
    "resumeTransfer()\0stopTheTransfer()\0"
    "skipCurrentTransfer(quint64)\0ids\0"
    "removeItems(QList<int>)\0"
    "moveItemsOnTop(QList<int>)\0"
    "moveItemsUp(QList<int>)\0"
    "moveItemsDown(QList<int>)\0"
    "moveItemsOnBottom(QList<int>)\0"
    "createNewWriteThread()\0newWriteThreadFinish()\0"
    "writeThreadOperationFinish()\0"
    "writeThreadOperationStart()\0"
    "startWriteThread(int)\0,\0"
    "manageTheWriteThreadError(QFileInfo,QString)\0"
    "pre_operation()\0post_operation()\0"
    "writeThreadIsFinish()\0"
    "timeOfTheBlockCopyFinished()\0"
};

void readThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        readThread *_t = static_cast<readThread *>(_o);
        switch (_id) {
        case 0: _t->queryNewWriteThread(); break;
        case 1: _t->queryStartWriteThread((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->fileAlreadyExists((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QFileInfo(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 3: _t->errorOnFile((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 4: _t->errorOnFileInWriteThread((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 5: _t->transferIsInPause((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->transferIsStarted((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->newTransferStart((*reinterpret_cast< ItemOfCopyList(*)>(_a[1]))); break;
        case 8: _t->newTransferStop((*reinterpret_cast< quint64(*)>(_a[1]))); break;
        case 9: _t->newActionOnList(); break;
        case 10: _t->debugInformation((*reinterpret_cast< DebugLevel(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5]))); break;
        case 11: _t->cancelAll(); break;
        case 12: _t->rmPath((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 13: _t->mkPath((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 14: _t->setRightTransfer((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 15: _t->setKeepDate((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 16: _t->setMaxSpeed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 17: _t->setPreallocateFileSize((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 18: { bool _r = _t->setBlockSize((*reinterpret_cast< const int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 19: _t->pauseTransfer(); break;
        case 20: _t->resumeTransfer(); break;
        case 21: _t->stopTheTransfer(); break;
        case 22: _t->skipCurrentTransfer((*reinterpret_cast< quint64(*)>(_a[1]))); break;
        case 23: _t->removeItems((*reinterpret_cast< QList<int>(*)>(_a[1]))); break;
        case 24: _t->moveItemsOnTop((*reinterpret_cast< QList<int>(*)>(_a[1]))); break;
        case 25: _t->moveItemsUp((*reinterpret_cast< QList<int>(*)>(_a[1]))); break;
        case 26: _t->moveItemsDown((*reinterpret_cast< QList<int>(*)>(_a[1]))); break;
        case 27: _t->moveItemsOnBottom((*reinterpret_cast< QList<int>(*)>(_a[1]))); break;
        case 28: _t->createNewWriteThread(); break;
        case 29: _t->newWriteThreadFinish(); break;
        case 30: _t->writeThreadOperationFinish(); break;
        case 31: _t->writeThreadOperationStart(); break;
        case 32: _t->startWriteThread((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 33: _t->manageTheWriteThreadError((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 34: _t->pre_operation(); break;
        case 35: _t->post_operation(); break;
        case 36: { bool _r = _t->writeThreadIsFinish();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 37: _t->timeOfTheBlockCopyFinished(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData readThread::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject readThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_readThread,
      qt_meta_data_readThread, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &readThread::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *readThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *readThread::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_readThread))
        return static_cast<void*>(const_cast< readThread*>(this));
    return QThread::qt_metacast(_clname);
}

int readThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 38)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 38;
    }
    return _id;
}

// SIGNAL 0
void readThread::queryNewWriteThread()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void readThread::queryStartWriteThread(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void readThread::fileAlreadyExists(QFileInfo _t1, QFileInfo _t2, bool _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void readThread::errorOnFile(QFileInfo _t1, QString _t2, bool _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void readThread::errorOnFileInWriteThread(QFileInfo _t1, QString _t2, int _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void readThread::transferIsInPause(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void readThread::transferIsStarted(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void readThread::newTransferStart(ItemOfCopyList _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void readThread::newTransferStop(quint64 _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void readThread::newActionOnList()
{
    QMetaObject::activate(this, &staticMetaObject, 9, 0);
}

// SIGNAL 10
void readThread::debugInformation(DebugLevel _t1, QString _t2, QString _t3, QString _t4, int _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void readThread::cancelAll()
{
    QMetaObject::activate(this, &staticMetaObject, 11, 0);
}

// SIGNAL 12
void readThread::rmPath(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void readThread::mkPath(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}
QT_END_MOC_NAMESPACE
