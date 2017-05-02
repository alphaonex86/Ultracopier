/** \file copyEngine.cpp
\brief Define the copy engine
 */

#include "copyEngineUnitTester.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

copyEngineUnitTester::copyEngineUnitTester(const QString &path,const QList<SupportedTest> &tests)
{
    this->path=path;
    connect(&timer,&QTimer::timeout,this,&copyEngineUnitTester::initialize,Qt::QueuedConnection);
    srand ( time(NULL) );
}

copyEngineUnitTester::~copyEngineUnitTester()
{
    rmpath(path);
}

void copyEngineUnitTester::errorSlot()
{
}

void copyEngineUnitTester::collisionSlot()
{
}

void copyEngineUnitTester::initialize()
{
    initializeSource();
}

void copyEngineUnitTester::initializeSource()
{
    QDir dir(path);
    dir.mkpath(path);
    dir.mkpath(path+"/source/");
    dir.mkpath(path+"/destination/");
}

bool copyEngineUnitTester::mkFile(const QString &dir,const quint16 &minSize,const quint16 &maxSize)
{
    QString name;
    int index=0;
    while(index<(16/4))
    {
        name+=QString::number(rand()%10000);
        index++;
    }
    QFile file(dir+"/"+name);
    if(file.open(QIODevice::WriteOnly))
    {
        int size=minSize+(rand()%(maxSize-minSize));
        int index=0;
        QByteArray byte;
        while(index<size)
        {
            byte[0]=rand()%256;
            file.write(byte);
            index++;
        }
        file.close();
        return true;
    }
    else
    {
        qDebug() << file.errorString();
        return false;
    }
}

/** remplace QDir::rmpath() because it return false if the folder not exists
  and seam bug with parent folder */
bool copyEngineUnitTester::rmpath(const QDir &dir)
{
    if(!dir.exists())
        return true;
    bool allHaveWork=true;
    QFileInfoList list = dir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo(list.at(i));
        if(!fileInfo.isDir())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"found a file: "+fileInfo.fileName());
            allHaveWork=false;
        }
        else
        {
            //return the fonction for scan the new folder
            if(!rmpath(dir.absolutePath()+'/'+fileInfo.fileName()+'/'))
                allHaveWork=false;
        }
    }
    if(!allHaveWork)
        return allHaveWork;
    allHaveWork=dir.rmdir(dir.absolutePath());
    if(!allHaveWork)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to remove the folder: "+dir.absolutePath());
    return allHaveWork;
}
