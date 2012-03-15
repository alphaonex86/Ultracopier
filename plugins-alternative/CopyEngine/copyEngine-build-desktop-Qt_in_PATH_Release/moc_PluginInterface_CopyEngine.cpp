/****************************************************************************
** Meta object code from reading C++ file 'PluginInterface_CopyEngine.h'
**
** Created: Wed Mar 7 11:15:01 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../interface/PluginInterface_CopyEngine.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PluginInterface_CopyEngine.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PluginInterface_CopyEngine[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      38,   33,   28,   27, 0x0a,
      62,   33,   28,   27, 0x0a,
      84,   27,   27,   27, 0x0a,
      92,   27,   27,   27, 0x0a,
     104,  101,   27,   27, 0x0a,
     118,   27,   27,   27, 0x0a,
     131,  127,   27,   27, 0x0a,
     155,  127,   27,   27, 0x0a,
     182,  127,   27,   27, 0x0a,
     206,  127,   27,   27, 0x0a,
     232,  127,   27,   27, 0x0a,
     262,   27,   27,   27, 0x0a,
     283,   27,   27,   27, 0x0a,
     320,  304,   28,   27, 0x0a,
     354,  347,   27,   27, 0x0a,
     382,  347,   27,   27, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_PluginInterface_CopyEngine[] = {
    "PluginInterface_CopyEngine\0\0bool\0mode\0"
    "userAddFolder(CopyMode)\0userAddFile(CopyMode)\0"
    "pause()\0resume()\0id\0skip(quint64)\0"
    "cancel()\0ids\0removeItems(QList<int>)\0"
    "moveItemsOnTop(QList<int>)\0"
    "moveItemsUp(QList<int>)\0"
    "moveItemsDown(QList<int>)\0"
    "moveItemsOnBottom(QList<int>)\0"
    "exportTransferList()\0importTransferList()\0"
    "speedLimitation\0setSpeedLimitation(qint64)\0"
    "action\0setCollisionAction(QString)\0"
    "setErrorAction(QString)\0"
};

void PluginInterface_CopyEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PluginInterface_CopyEngine *_t = static_cast<PluginInterface_CopyEngine *>(_o);
        switch (_id) {
        case 0: { bool _r = _t->userAddFolder((*reinterpret_cast< const CopyMode(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 1: { bool _r = _t->userAddFile((*reinterpret_cast< const CopyMode(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 2: _t->pause(); break;
        case 3: _t->resume(); break;
        case 4: _t->skip((*reinterpret_cast< const quint64(*)>(_a[1]))); break;
        case 5: _t->cancel(); break;
        case 6: _t->removeItems((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 7: _t->moveItemsOnTop((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 8: _t->moveItemsUp((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 9: _t->moveItemsDown((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 10: _t->moveItemsOnBottom((*reinterpret_cast< const QList<int>(*)>(_a[1]))); break;
        case 11: _t->exportTransferList(); break;
        case 12: _t->importTransferList(); break;
        case 13: { bool _r = _t->setSpeedLimitation((*reinterpret_cast< const qint64(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 14: _t->setCollisionAction((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 15: _t->setErrorAction((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData PluginInterface_CopyEngine::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PluginInterface_CopyEngine::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_PluginInterface_CopyEngine,
      qt_meta_data_PluginInterface_CopyEngine, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PluginInterface_CopyEngine::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PluginInterface_CopyEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PluginInterface_CopyEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PluginInterface_CopyEngine))
        return static_cast<void*>(const_cast< PluginInterface_CopyEngine*>(this));
    return QObject::qt_metacast(_clname);
}

int PluginInterface_CopyEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}
static const uint qt_meta_data_PluginInterface_CopyEngineFactory[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      35,   34,   34,   34, 0x0a,
      50,   34,   34,   34, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_PluginInterface_CopyEngineFactory[] = {
    "PluginInterface_CopyEngineFactory\0\0"
    "resetOptions()\0newLanguageLoaded()\0"
};

void PluginInterface_CopyEngineFactory::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PluginInterface_CopyEngineFactory *_t = static_cast<PluginInterface_CopyEngineFactory *>(_o);
        switch (_id) {
        case 0: _t->resetOptions(); break;
        case 1: _t->newLanguageLoaded(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData PluginInterface_CopyEngineFactory::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PluginInterface_CopyEngineFactory::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_PluginInterface_CopyEngineFactory,
      qt_meta_data_PluginInterface_CopyEngineFactory, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PluginInterface_CopyEngineFactory::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PluginInterface_CopyEngineFactory::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PluginInterface_CopyEngineFactory::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PluginInterface_CopyEngineFactory))
        return static_cast<void*>(const_cast< PluginInterface_CopyEngineFactory*>(this));
    return QObject::qt_metacast(_clname);
}

int PluginInterface_CopyEngineFactory::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
