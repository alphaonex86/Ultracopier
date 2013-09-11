/** \file copyEngine.h
\brief Define the copy engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include <QFile>
#include <QTimer>

#include "copyEngine.h"

#ifndef COPY_ENGINE_UNIT_TESTER_H
#define COPY_ENGINE_UNIT_TESTER_H

namespace Ui {
    class options;
}

/// \brief the implementation of copy engine plugin, manage directly few stuff, else pass to ListThread class.
class copyEngineUnitTester : public copyEngine
{
        Q_OBJECT
public:
    enum SupportedTest{Test_Copy};

    copyEngineUnitTester(const QString &path,const QList<SupportedTest> &tests);
    ~copyEngineUnitTester();
private:
    ListThread *listThread;
    bool				dialogIsOpen;
    QList<SupportedTest> tests;
    QTimer timer;
    QString path;
    void initializeSource();
    bool rmpath(const QDir &dir);
    bool mkFile(const QString &dir,const quint16 &minSize=0,const quint16 &maxSize=65535);
private slots:
    void errorSlot();
    void collisionSlot();
    void initialize();
};

#endif // COPY_ENGINE_UNIT_TESTER_H
