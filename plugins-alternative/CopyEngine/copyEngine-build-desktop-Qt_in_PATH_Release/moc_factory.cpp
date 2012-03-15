/****************************************************************************
** Meta object code from reading C++ file 'factory.h'
**
** Created: Wed Mar 7 11:14:59 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Ultracopier-0.2/factory.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'factory.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Factory[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
       9,    8,    8,    8, 0x05,
      57,   26,    8,    8, 0x05,

 // slots: signature, parameters, type, tag, flags
     120,  114,    8,    8, 0x08,
     170,  150,    8,    8, 0x08,
     205,    8,    8,    8, 0x08,
     230,    8,    8,    8, 0x08,
     272,  256,    8,    8, 0x08,
     306,  297,    8,    8, 0x08,
     333,  324,    8,    8, 0x08,
     361,  351,    8,    8, 0x08,
     389,  379,    8,    8, 0x08,
     431,  408,    8,    8, 0x08,
     469,  463,    8,    8, 0x08,
     520,  463,    8,    8, 0x08,
     568,    8,    8,    8, 0x0a,
     583,    8,    8,    8, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Factory[] = {
    "Factory\0\0reloadLanguage()\0"
    "level,fonction,text,file,ligne\0"
    "debugInformation(DebugLevel,QString,QString,QString,int)\0"
    "error\0error(QProcess::ProcessError)\0"
    "exitCode,exitStatus\0"
    "finished(int,QProcess::ExitStatus)\0"
    "readyReadStandardError()\0"
    "readyReadStandardOutput()\0doRightTransfer\0"
    "setDoRightTransfer(bool)\0keepDate\0"
    "setKeepDate(bool)\0prealloc\0setPrealloc(bool)\0"
    "blockSize\0setBlockSize(int)\0autoStart\0"
    "setAutoStart(bool)\0checkDestinationFolder\0"
    "setCheckDestinationFolder(bool)\0index\0"
    "on_comboBoxFolderColision_currentIndexChanged(int)\0"
    "on_comboBoxFolderError_currentIndexChanged(int)\0"
    "resetOptions()\0newLanguageLoaded()\0"
};

void Factory::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Factory *_t = static_cast<Factory *>(_o);
        switch (_id) {
        case 0: _t->reloadLanguage(); break;
        case 1: _t->debugInformation((*reinterpret_cast< DebugLevel(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5]))); break;
        case 2: _t->error((*reinterpret_cast< QProcess::ProcessError(*)>(_a[1]))); break;
        case 3: _t->finished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        case 4: _t->readyReadStandardError(); break;
        case 5: _t->readyReadStandardOutput(); break;
        case 6: _t->setDoRightTransfer((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->setKeepDate((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->setPrealloc((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->setBlockSize((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->setAutoStart((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->setCheckDestinationFolder((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->on_comboBoxFolderColision_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->on_comboBoxFolderError_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->resetOptions(); break;
        case 15: _t->newLanguageLoaded(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Factory::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Factory::staticMetaObject = {
    { &PluginInterface_CopyEngineFactory::staticMetaObject, qt_meta_stringdata_Factory,
      qt_meta_data_Factory, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Factory::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Factory::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Factory::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Factory))
        return static_cast<void*>(const_cast< Factory*>(this));
    if (!strcmp(_clname, "first-world.info.ultracopier.PluginInterface.CopyEngineFactory/0.3.0.4"))
        return static_cast< PluginInterface_CopyEngineFactory*>(const_cast< Factory*>(this));
    return PluginInterface_CopyEngineFactory::qt_metacast(_clname);
}

int Factory::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = PluginInterface_CopyEngineFactory::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void Factory::reloadLanguage()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void Factory::debugInformation(DebugLevel _t1, QString _t2, QString _t3, QString _t4, int _t5)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
