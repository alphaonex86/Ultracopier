/****************************************************************************
** Meta object code from reading C++ file 'fileIsSameDialog.h'
**
** Created: Wed Mar 7 11:14:59 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Ultracopier-0.2/fileIsSameDialog.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fileIsSameDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_fileIsSameDialog[] = {

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
      18,   17,   17,   17, 0x08,
      46,   17,   17,   17, 0x08,
      66,   17,   17,   17, 0x08,
      84,   17,   17,   17, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_fileIsSameDialog[] = {
    "fileIsSameDialog\0\0on_SuggestNewName_clicked()\0"
    "on_Rename_clicked()\0on_Skip_clicked()\0"
    "on_Cancel_clicked()\0"
};

void fileIsSameDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        fileIsSameDialog *_t = static_cast<fileIsSameDialog *>(_o);
        switch (_id) {
        case 0: _t->on_SuggestNewName_clicked(); break;
        case 1: _t->on_Rename_clicked(); break;
        case 2: _t->on_Skip_clicked(); break;
        case 3: _t->on_Cancel_clicked(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData fileIsSameDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject fileIsSameDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_fileIsSameDialog,
      qt_meta_data_fileIsSameDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &fileIsSameDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *fileIsSameDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *fileIsSameDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_fileIsSameDialog))
        return static_cast<void*>(const_cast< fileIsSameDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int fileIsSameDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
