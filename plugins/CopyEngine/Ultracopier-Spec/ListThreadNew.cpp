#include "ListThread.h"
#include <QStorageInfo>
#include <QtGlobal>
#include "../../../cpp11addition.h"

#include "async/TransferThreadAsync.h"

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::newCopy(const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sources: "+stringimplode(sources,";")+", destination: "+destination);
    ScanFileOrFolder * scanFileOrFolderThread=newScanThread(Ultracopier::Copy);
    if(scanFileOrFolderThread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to get new thread");
        return false;
    }
    std::regex base_regex("^[a-z][a-z][a-z]*:/.*");
    std::smatch base_match;
    std::vector<INTERNALTYPEPATH> sourcesClean;
    unsigned int index=0;
    while(index<sources.size())
    {
        std::string source=sources.at(index);
        //can be: file://192.168.0.99/share/file.txt
        //can be: file:///C:/file.txt
        //can be: file:///home/user/fileatrootunderunix
        #ifndef Q_OS_WIN
        if(stringStartWith(source,"file:///"))
            source.replace(0,7,"");
        #else
        if(stringStartWith(source,"file:///"))
            source.replace(0,8,"");
        else if(stringStartWith(source,"file://"))
            source.replace(0,5,"");
        else if(stringStartWith(source,"file:/"))
            source.replace(0,6,"");
        #endif
        else if (std::regex_match(source, base_match, base_regex))
            return false;
        if(index<99)
        {
            if(sources.at(index)!=source)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,sources.at(index)+" -> "+source);
        }
        index++;
        sourcesClean.push_back(TransferThread::stringToInternalString(source));
    }
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"sourcesClean: "+stringimplode(sourcesClean,";"));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination: "+destination);
    const INTERNALTYPEPATH &Wdest=TransferThread::stringToInternalString(destination);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination: "+TransferThread::internalStringTostring(Wdest));
    scanFileOrFolderThread->addToList(sourcesClean,Wdest);
    scanThreadHaveFinish(true);
    detectDrivesOfCurrentTransfer(sourcesClean,TransferThread::stringToInternalString(destination));
    return true;
}

// -> add thread safe, by Qt::BlockingQueuedConnection
bool ListThread::newMove(const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    {
        unsigned int index=0;
        while(index<sources.size())
        {
            std::string source=sources.at(index);
            //can be: file://192.168.0.99/share/file.txt
            //can be: file:///C:/file.txt
            //can be: file:///home/user/fileatrootunderunix
            #ifndef Q_OS_WIN
            if(stringStartWith(source,"file:///"))
                source.replace(0,7,"");
            #else
            if(stringStartWith(source,"file:///"))
                source.replace(0,8,"");
            else if(stringStartWith(source,"file://"))
                source.replace(0,5,"");
            else if(stringStartWith(source,"file:/"))
                source.replace(0,6,"");
            #endif
            if(index<99)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,sources.at(index)+" -> "+source);
            index++;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source is_file: "+std::to_string(TransferThread::is_file(TransferThread::stringToInternalString(source)))+" is_dir: "+std::to_string(TransferThread::is_dir(TransferThread::stringToInternalString(source))));
        }
    }
    #endif

    ScanFileOrFolder * scanFileOrFolderThread = newScanThread(Ultracopier::Move);
    if(scanFileOrFolderThread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to get new thread");
        return false;
    }
    std::regex base_regex("^[a-z][a-z][a-z]*:/.*");
    std::smatch base_match;
    std::vector<INTERNALTYPEPATH> sourcesClean;
    unsigned int index=0;
    while(index<sources.size())
    {
        std::string source=sources.at(index);
        //can be: file://192.168.0.99/share/file.txt
        //can be: file:///C:/file.txt
        //can be: file:///home/user/fileatrootunderunix
        #ifndef Q_OS_WIN
        if(stringStartWith(source,"file:///"))
            source.replace(0,7,"");
        #else
        if(stringStartWith(source,"file:///"))
            source.replace(0,8,"");
        else if(stringStartWith(source,"file://"))
            source.replace(0,5,"");
        else if(stringStartWith(source,"file:/"))
            source.replace(0,6,"");
        #endif
        else if (std::regex_match(source, base_match, base_regex))
            return false;
        if(index<99)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,sources.at(index)+" -> "+source);
        index++;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source is_file: "+std::to_string(TransferThread::is_file(TransferThread::stringToInternalString(source)))+" is_dir: "+std::to_string(TransferThread::is_dir(TransferThread::stringToInternalString(source))));
        sourcesClean.push_back(TransferThread::stringToInternalString(source));
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination: "+destination);
    const INTERNALTYPEPATH &Wdest=TransferThread::stringToInternalString(destination);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination: "+TransferThread::internalStringTostring(Wdest));
    scanFileOrFolderThread->addToList(sourcesClean,Wdest);
    scanThreadHaveFinish(true);
    detectDrivesOfCurrentTransfer(sourcesClean,TransferThread::stringToInternalString(destination));
    return true;
}
