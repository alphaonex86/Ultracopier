/****************************************************************************
** Meta object code from reading C++ file 'copyEngine.h'
**
** Created: Wed Mar 7 11:14:55 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Ultracopier-0.2/copyEngine.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'copyEngine.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_copyEngine[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      55,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: signature, parameters, type, tag, flags
      43,   12,   11,   11, 0x05,
     123,  100,   11,   11, 0x05,
     178,  163,   11,   11, 0x05,
     214,  211,   11,   11, 0x05,
     244,  239,   11,   11, 0x05,
     277,  270,   11,   11, 0x05,
     305,  270,   11,   11, 0x05,
     339,  329,   11,   11, 0x05,
     355,   11,   11,   11, 0x05,
     373,   11,   11,   11, 0x05,
     407,  385,   11,   11, 0x05,
     448,  239,   11,   11, 0x05,
     464,  239,   11,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
     490,  485,  480,   11, 0x0a,
     514,  485,  480,   11, 0x0a,
     536,   11,   11,   11, 0x0a,
     544,   11,   11,   11, 0x0a,
     553,  211,   11,   11, 0x0a,
     567,   11,   11,   11, 0x0a,
     580,  576,   11,   11, 0x0a,
     604,  576,   11,   11, 0x0a,
     631,  576,   11,   11, 0x0a,
     655,  576,   11,   11, 0x0a,
     681,  576,   11,   11, 0x0a,
     711,   11,   11,   11, 0x0a,
     732,   11,   11,   11, 0x0a,
     769,  753,  480,   11, 0x0a,
     796,  270,   11,   11, 0x0a,
     824,  270,   11,   11, 0x0a,
     864,  848,   11,   11, 0x0a,
     896,  887,   11,   11, 0x0a,
     923,  914,   11,   11, 0x0a,
     949,  940,   11,   11, 0x0a,
     988,  978,   11,   11, 0x0a,
    1016, 1006,   11,   11, 0x0a,
    1064, 1035,   11,   11, 0x0a,
    1102,   11,   11,   11, 0x0a,
    1122,   11,   11,   11, 0x0a,
    1146, 1140,   11,   11, 0x0a,
    1197, 1140,   11,   11, 0x0a,
    1261, 1245,   11,   11, 0x08,
    1288,   11,   11,   11, 0x28,
    1311,   11,   11,   11, 0x08,
    1329,   11,   11,   11, 0x08,
    1385, 1353,   11,   11, 0x08,
    1456, 1421,   11,   11, 0x08,
    1516, 1490,   11,   11, 0x08,
    1596, 1560,   11,   11, 0x08,
    1672, 1632,   11,   11, 0x08,
    1746, 1720,   11,   11, 0x08,
    1813, 1792,   11,   11, 0x08,
    1868, 1846,   11,   11, 0x08,
    1919,  270,   11,   11, 0x28,
    1965, 1846,   11,   11, 0x08,
    2010,  270,   11,   11, 0x28,

       0        // eod
};

static const char qt_meta_stringdata_copyEngine[] = {
    "copyEngine\0\0level,fonction,text,file,ligne\0"
    "debugInformation(DebugLevel,QString,QString,QString,int)\0"
    "engineActionInProgress\0"
    "actionInProgess(EngineActionInProgress)\0"
    "itemOfCopyList\0newTransferStart(ItemOfCopyList)\0"
    "id\0newTransferStop(quint64)\0path\0"
    "newFolderListing(QString)\0action\0"
    "newCollisionAction(QString)\0"
    "newErrorAction(QString)\0isInPause\0"
    "isInPause(bool)\0newActionOnList()\0"
    "cancelAll()\0path,size,mtime,error\0"
    "error(QString,quint64,QDateTime,QString)\0"
    "rmPath(QString)\0mkPath(QString)\0bool\0"
    "mode\0userAddFolder(CopyMode)\0"
    "userAddFile(CopyMode)\0pause()\0resume()\0"
    "skip(quint64)\0cancel()\0ids\0"
    "removeItems(QList<int>)\0"
    "moveItemsOnTop(QList<int>)\0"
    "moveItemsUp(QList<int>)\0"
    "moveItemsDown(QList<int>)\0"
    "moveItemsOnBottom(QList<int>)\0"
    "exportTransferList()\0importTransferList()\0"
    "speedLimitation\0setSpeedLimitation(qint64)\0"
    "setCollisionAction(QString)\0"
    "setErrorAction(QString)\0doRightTransfer\0"
    "setRightTransfer(bool)\0keepDate\0"
    "setKeepDate(bool)\0maxSpeed\0setMaxSpeed(int)\0"
    "prealloc\0setPreallocateFileSize(bool)\0"
    "blockSize\0setBlockSize(int)\0autoStart\0"
    "setAutoStart(bool)\0checkDestinationFolderExists\0"
    "setCheckDestinationFolderExists(bool)\0"
    "newLanguageLoaded()\0resetTempWidget()\0"
    "index\0on_comboBoxFolderColision_currentIndexChanged(int)\0"
    "on_comboBoxFolderError_currentIndexChanged(int)\0"
    "skipFirstRemove\0scanThreadHaveFinish(bool)\0"
    "scanThreadHaveFinish()\0updateTheStatus()\0"
    "transferIsStarted(bool)\0"
    "source,destination,numberOfItem\0"
    "folderTransfer(QString,QString,int)\0"
    "sourceFileInfo,destinationFileInfo\0"
    "fileTransfer(QFileInfo,QFileInfo)\0"
    "source,destination,isSame\0"
    "fileAlreadyExists(QFileInfo,QFileInfo,bool)\0"
    "fileInfo,errorString,canPutAtTheEnd\0"
    "errorOnFile(QFileInfo,QString,bool)\0"
    "fileInfo,errorString,indexOfWriteThread\0"
    "errorOnFileInWriteThread(QFileInfo,QString,int)\0"
    "source,isSame,destination\0"
    "folderAlreadyExists(QFileInfo,bool,QFileInfo)\0"
    "fileInfo,errorString\0"
    "errorOnFolder(QFileInfo,QString)\0"
    "action,changeComboBox\0"
    "setComboBoxFolderColision(FolderExistsAction,bool)\0"
    "setComboBoxFolderColision(FolderExistsAction)\0"
    "setComboBoxFolderError(FileErrorAction,bool)\0"
    "setComboBoxFolderError(FileErrorAction)\0"
};

void copyEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        copyEngine *_t = static_cast<copyEngine *>(_o);
        switch (_id) {
        case 0: _t->debugInformation((*reinterpret_cast< DebugLevel(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5]))); break;
        case 1: _t->actionInProgess((*reinterpret_cast< EngineActionInProgress(*)>(_a[1]))); break;
        case 2: _t->newTransferStart((*reinterpret_cast< ItemOfCopyList(*)>(_a[1]))); break;
        case 3: _t->newTransferStop((*reinterpret_cast< quint64(*)>(_a[1]))); break;
        case 4: _t->newFolderListing((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 5: _t->newCollisionAction((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 6: _t->newErrorAction((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->isInPause((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->newActionOnList(); break;
        case 9: _t->cancelAll(); break;
        case 10: _t->error((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< quint64(*)>(_a[2])),(*reinterpret_cast< QDateTime(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4]))); break;
        case 11: _t->rmPath((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 12: _t->mkPath((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 13: { bool _r = _t->userAddFolder((*reinterpret_cast< const CopyMode(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 14: { bool _r = _t->userAddFile((*reinterpret_cast< const CopyMode(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 15: _t->pause(); break;
        case 16: _t->resume(); break;
        case 17: _t->skip((*reinterpret_cast< const quint64(*)>(_a[1]))); break;
        case 18: _t->cancel(); break;
        case 19: _t->removeItems((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 20: _t->moveItemsOnTop((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 21: _t->moveItemsUp((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 22: _t->moveItemsDown((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 23: _t->moveItemsOnBottom((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 24: _t->exportTransferList(); break;
        case 25: _t->importTransferList(); break;
        case 26: { bool _r = _t->setSpeedLimitation((*reinterpret_cast< const qint64(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 27: _t->setCollisionAction((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 28: _t->setErrorAction((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 29: _t->setRightTransfer((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 30: _t->setKeepDate((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 31: _t->setMaxSpeed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 32: _t->setPreallocateFileSize((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 33: _t->setBlockSize((*reinterpret_cast< const int(*)>(_a[1]))); break;
        case 34: _t->setAutoStart((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 35: _t->setCheckDestinationFolderExists((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 36: _t->newLanguageLoaded(); break;
        case 37: _t->resetTempWidget(); break;
        case 38: _t->on_comboBoxFolderColision_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 39: _t->on_comboBoxFolderError_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 40: _t->scanThreadHaveFinish((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 41: _t->scanThreadHaveFinish(); break;
        case 42: _t->updateTheStatus(); break;
        case 43: _t->transferIsStarted((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 44: _t->folderTransfer((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 45: _t->fileTransfer((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QFileInfo(*)>(_a[2]))); break;
        case 46: _t->fileAlreadyExists((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QFileInfo(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 47: _t->errorOnFile((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 48: _t->errorOnFileInWriteThread((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 49: _t->folderAlreadyExists((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< QFileInfo(*)>(_a[3]))); break;
        case 50: _t->errorOnFolder((*reinterpret_cast< QFileInfo(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 51: _t->setComboBoxFolderColision((*reinterpret_cast< FolderExistsAction(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 52: _t->setComboBoxFolderColision((*reinterpret_cast< FolderExistsAction(*)>(_a[1]))); break;
        case 53: _t->setComboBoxFolderError((*reinterpret_cast< FileErrorAction(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 54: _t->setComboBoxFolderError((*reinterpret_cast< FileErrorAction(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData copyEngine::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject copyEngine::staticMetaObject = {
    { &PluginInterface_CopyEngine::staticMetaObject, qt_meta_stringdata_copyEngine,
      qt_meta_data_copyEngine, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &copyEngine::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *copyEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *copyEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_copyEngine))
        return static_cast<void*>(const_cast< copyEngine*>(this));
    return PluginInterface_CopyEngine::qt_metacast(_clname);
}

int copyEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PluginInterface_CopyEngine::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 55)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 55;
    }
    return _id;
}

// SIGNAL 0
void copyEngine::debugInformation(DebugLevel _t1, QString _t2, QString _t3, QString _t4, int _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void copyEngine::actionInProgess(EngineActionInProgress _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void copyEngine::newTransferStart(ItemOfCopyList _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void copyEngine::newTransferStop(quint64 _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void copyEngine::newFolderListing(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void copyEngine::newCollisionAction(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void copyEngine::newErrorAction(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void copyEngine::isInPause(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void copyEngine::newActionOnList()
{
    QMetaObject::activate(this, &staticMetaObject, 8, 0);
}

// SIGNAL 9
void copyEngine::cancelAll()
{
    QMetaObject::activate(this, &staticMetaObject, 9, 0);
}

// SIGNAL 10
void copyEngine::error(QString _t1, quint64 _t2, QDateTime _t3, QString _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void copyEngine::rmPath(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void copyEngine::mkPath(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}
QT_END_MOC_NAMESPACE
