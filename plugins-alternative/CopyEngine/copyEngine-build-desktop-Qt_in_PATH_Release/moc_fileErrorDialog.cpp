/****************************************************************************
** Meta object code from reading C++ file 'fileErrorDialog.h'
**
** Created: Wed Mar 7 11:14:57 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Ultracopier-0.2/fileErrorDialog.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fileErrorDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_fileErrorDialog[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      17,   16,   16,   16, 0x08,
      42,   16,   16,   16, 0x08,
      61,   16,   16,   16, 0x08,
      79,   16,   16,   16, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_fileErrorDialog[] = {
    "fileErrorDialog\0\0on_PutToBottom_clicked()\0"
    "on_Retry_clicked()\0on_Skip_clicked()\0"
    "on_Cancel_clicked()\0"
};

void fileErrorDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        fileErrorDialog *_t = static_cast<fileErrorDialog *>(_o);
        switch (_id) {
        case 0: _t->on_PutToBottom_clicked(); break;
        case 1: _t->on_Retry_clicked(); break;
        case 2: _t->on_Skip_clicked(); break;
        case 3: _t->on_Cancel_clicked(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData fileErrorDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject fileErrorDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_fileErrorDialog,
      qt_meta_data_fileErrorDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &fileErrorDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *fileErrorDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *fileErrorDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_fileErrorDialog))
        return static_cast<void*>(const_cast< fileErrorDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int fileErrorDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
