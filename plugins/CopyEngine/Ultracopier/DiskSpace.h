#ifndef DISKSPACE_H
#define DISKSPACE_H

#include <QDialog>
#include "../../../interface/PluginInterface_CopyEngine.h"
#include "StructEnumDefinition_CopyEngine.h"

namespace Ui {
class DiskSpace;
}

class DiskSpace : public QDialog
{
    Q_OBJECT

public:
    explicit DiskSpace(FacilityInterface * facilityEngine,QList<Diskspace> list,QWidget *parent = 0);
    ~DiskSpace();
    bool getAction() const;
private slots:
    void on_ok_clicked();
    void on_cancel_clicked();
private:
    Ui::DiskSpace *ui;
    bool ok;
};

#endif // DISKSPACE_H
